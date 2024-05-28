#include <pistache/endpoint.h>
#include <pistache/router.h>
#include "connection.h"
#include <libpq-fe.h>
#include <database.h>
#include <nlohmann/json.hpp>
#include <opencv2/core/hal/interface.h>
#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <iostream>
#include <login.h>

using json = nlohmann::json;
using namespace Pistache;

struct User login_user(PGconn *conn, const char *username, const char *password, int *returnCode)
{
    struct User user;
    const char *params[] = {username, password};
    const char *query = "SELECT id, name FROM users WHERE name = $1 AND password = $2";
    PGresult *res = PQexecParams(conn, query, 2, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        if (PQresultStatus(res) == PGRES_NONFATAL_ERROR)
        {
            fprintf(stderr, "Error executing query: %s\n", PQerrorMessage(conn));
        }
        PQclear(res);
        *returnCode = 1;
        return user;
    }

    int num_rows = PQntuples(res);

    if (num_rows != 1)
    {
        PQclear(res);
        return user;
    }
    user.id = atoi(PQgetvalue(res, 0, 0));
    strcpy(user.name, PQgetvalue(res, 0, 1));

    *returnCode = 0;
    PQclear(res);
    return user;
}

unsigned char *encodeImage2(const cv::Mat &image)
{
    std::vector<uchar> buffer;
    cv::imencode(".jpg", image, buffer);
    unsigned char *encodedData = new unsigned char[buffer.size()];
    std::copy(buffer.begin(), buffer.end(), encodedData);
    return encodedData;
}

cv::Mat decodeImage2(const unsigned char *data, size_t size)
{
    std::vector<uchar> buffer(data, data + size);
    return cv::imdecode(buffer, cv::IMREAD_COLOR);
}

unsigned char *applyNegative(const unsigned char *data, size_t size)
{
    cv::Mat image = decodeImage2(data, size);
    if (image.empty())
    {
        std::stringstream ss;
        for (size_t i = 0; i < size; ++i)
        {
            ss << static_cast<int>(data[i]) << " ";
        }
        std::string imageDataStr = ss.str();

        std::string errorMessage = "Decoded image is empty. Image size: " + std::to_string(size) + ". Image data: " + imageDataStr;
        throw std::runtime_error(errorMessage);
    }

    cv::Mat result;
    cv::bitwise_not(image, result);
    return encodeImage2(result);
}

std::vector<unsigned char> base64_decode(const std::string &encoded)
{
    static constexpr unsigned char kDecodingTable[] = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
        64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64};

    size_t in_len = encoded.size();
    if (in_len % 4 != 0)
    {
        throw std::runtime_error("Invalid base64 input");
    }

    size_t out_len = in_len / 4 * 3;
    if (encoded[in_len - 1] == '=')
    {
        out_len--;
    }
    if (encoded[in_len - 2] == '=')
    {
        out_len--;
    }

    std::vector<unsigned char> decoded(out_len);
    for (size_t i = 0, j = 0; i < in_len;)
    {
        unsigned char a = encoded[i] == '=' ? 0 & i++ : kDecodingTable[encoded[i++] - 43];
        unsigned char b = encoded[i] == '=' ? 0 & i++ : kDecodingTable[encoded[i++] - 43];
        unsigned char c = encoded[i] == '=' ? 0 & i++ : kDecodingTable[encoded[i++] - 43];
        unsigned char d = encoded[i] == '=' ? 0 & i++ : kDecodingTable[encoded[i++] - 43];

        decoded[j++] = (a << 2) | (b >> 4);
        if (j < out_len)
        {
            decoded[j++] = (b << 4) | (c >> 2);
        }
        if (j < out_len)
        {
            decoded[j++] = (c << 6) | d;
        }
    }

    return decoded;
}

std::string base64_encode(const unsigned char *data, size_t length)
{
    static constexpr char kEncodingTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string encoded;
    encoded.reserve(((length + 2) / 3) * 4);

    for (size_t i = 0; i < length;)
    {
        unsigned char a = i < length ? data[i++] : 0;
        unsigned char b = i < length ? data[i++] : 0;
        unsigned char c = i < length ? data[i++] : 0;

        encoded.push_back(kEncodingTable[a >> 2]);
        encoded.push_back(kEncodingTable[((a & 0x3) << 4) | (b >> 4)]);
        encoded.push_back(kEncodingTable[((b & 0xf) << 2) | (c >> 6)]);
        encoded.push_back(kEncodingTable[c & 0x3f]);
    }

    while (encoded.length() % 4)
    {
        encoded.push_back('=');
    }

    return encoded;
}

bool register_user(PGconn *conn, const char *username, const char *password)
{
    if (username == NULL || username[0] == '\0' || password == NULL || password[0] == '\0')
    {
        fprintf(stderr, "Error: Username and password cannot be empty.\n");
        return false;
    }

    const char *params[] = {username, password};
    const char *query = "INSERT INTO users (name, password) VALUES ($1, $2)";

    PGresult *res = PQexecParams(conn, query, 2, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        if (PQresultStatus(res) == PGRES_NONFATAL_ERROR)
        {
            fprintf(stderr, "Error executing query: %s\n", PQerrorMessage(conn));
        }
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

bool like_or_remove_like(PGconn *conn, int user_id, int post_id)
{
    char query[100];
    snprintf(query, sizeof(query), "SELECT ID FROM user_liked_post WHERE user_id = %d AND post_id = %d", user_id, post_id);

    PGresult *res = PQexec(conn, query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "Error executing query: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) > 0)
    {
        // Like exists, so delete it
        char delete_query[100];
        snprintf(delete_query, sizeof(delete_query), "DELETE FROM user_liked_post WHERE user_id = %d AND post_id = %d", user_id, post_id);

        PGresult *delete_res = PQexec(conn, delete_query);

        if (PQresultStatus(delete_res) != PGRES_COMMAND_OK)
        {
            fprintf(stderr, "Error executing delete query: %s\n", PQerrorMessage(conn));
            PQclear(delete_res);
            PQclear(res);
            return -1;
        }

        PQclear(delete_res);
    }
    else
    {
        char insert_query[100];
        snprintf(insert_query, sizeof(insert_query), "INSERT INTO user_liked_post (post_id, user_id) VALUES (%d, %d)", post_id, user_id);

        PGresult *insert_res = PQexec(conn, insert_query);

        if (PQresultStatus(insert_res) != PGRES_COMMAND_OK)
        {
            fprintf(stderr, "Error executing insert query: %s\n", PQerrorMessage(conn));
            PQclear(insert_res);
            PQclear(res);
            return -1;
        }

        PQclear(insert_res);
    }

    PQclear(res);
    return true;
}

unsigned char hexCharToByte2(char hex)
{
    if (hex >= '0' && hex <= '9')
    {
        return hex - '0';
    }
    else if (hex >= 'a' && hex <= 'f')
    {
        return hex - 'a' + 10;
    }
    else if (hex >= 'A' && hex <= 'F')
    {
        return hex - 'A' + 10;
    }
    return 0;
}

std::vector<unsigned char> transformData2(const std::vector<unsigned char> &inputData)
{
    std::vector<unsigned char> outputData;
    for (size_t i = 2; i < inputData.size(); i += 2)
    {
        unsigned char value = (hexCharToByte2(inputData[i]) << 4) | hexCharToByte2(inputData[i + 1]);
        outputData.push_back(value);
    }
    return outputData;
}

std::vector<uchar> convertToVector2(const uchar *data, size_t size)
{
    return std::vector<uchar>(data, data + size);
}

class HelloHandler : public Http::Handler
{
public:
    HTTP_PROTOTYPE(HelloHandler)

    HelloHandler()
    {
        const char *connstring = "host=dpg-cohr28ol5elc73csm2i0-a.frankfurt-postgres.render.com port=5432 dbname=pcd user=pcd_user password=OAGPeU3TKCHQ3hePtl69HSQNb8DiBbls";
        conn = PQconnectdb(connstring);
        if (PQstatus(conn) != CONNECTION_OK)
        {
            fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
            PQfinish(conn);
            conn = nullptr;
        }
    }

    ~HelloHandler()
    {
        if (conn)
        {
            PQfinish(conn);
        }
    }

    void onRequest(const Http::Request &request, Http::ResponseWriter response) override
    {
        Http::Header::Collection &headers = response.headers();
        headers.add<Http::Header::AccessControlAllowOrigin>("*");
        headers.add<Http::Header::AccessControlAllowMethods>("GET, POST, PUT, DELETE, OPTIONS");
        headers.add<Http::Header::AccessControlAllowHeaders>("Content-Type, Accept, application/json");
        // if (!conn || PQstatus(conn) != CONNECTION_OK)
        // {
        //     response.send(Http::Code::Service_Unavailable, "Database connection failed\n");
        //     return;
        // }

        if (request.resource() == "/")
        {
            response.send(Http::Code::Ok, "Hello, World\n");
        }
        else if (request.resource() == "/login" && request.method() == Http::Method::Post)
        {
            auto body = request.body();
            auto jsonBody = json::parse(body);
            int loginResult = 1;

            std::string username = jsonBody.at("username").get<std::string>();
            std::string password = jsonBody.at("password").get<std::string>();

            char *c_username = (char *)username.c_str();
            char *c_password = (char *)password.c_str();

            struct User user = login_user(conn, c_username, c_password, &loginResult);

            json postJson;

            if (loginResult)
            {
                json postJson = loginResult;
                response.send(Http::Code::Unauthorized, postJson.dump());
            }
            else
            {
                postJson["id"] = user.id;
                postJson["name"] = std::string(user.name);
                response.send(Http::Code::Ok, postJson.dump());
                memset(c_username, 0, sizeof(c_username));
                memset(c_password, 0, sizeof(c_password));
            }
        }
        else if (request.resource() == "/register" && request.method() == Http::Method::Post)
        {
            auto body = request.body();
            auto jsonBody = json::parse(body);
            int loginResult = 1;

            std::string username = jsonBody.at("username").get<std::string>();
            std::string password = jsonBody.at("password").get<std::string>();

            char *c_username = (char *)username.c_str();
            char *c_password = (char *)password.c_str();

            bool status = register_user(conn, c_username, c_password);
            if (status)
            {
                json postJson = status;
                response.send(Http::Code::Ok, postJson.dump());
            }
            else
            {
                json postJson = status;
                response.send(Http::Code::Conflict, postJson.dump());
            }
        }

        else if (request.resource() == "/like" && request.method() == Http::Method::Post)
        {
            auto body = request.body();
            auto jsonBody = json::parse(body);
            int loginResult = 1;

            int postId = jsonBody.at("postId").get<int>();
            int userId = jsonBody.at("userId").get<int>();

            bool status = like_or_remove_like(conn, userId, postId);
            json postJson = status;
            response.send(Http::Code::Ok, postJson.dump());
        }

        else if (request.resource() == "/posts")
        {
            struct Post *posts = get_all_posts(conn, 2);
            if (posts)
            {
                json postsJson;
                for (int i = 0; posts[i].id != 0; ++i)
                {
                    printf("%s", posts[i].description);
                    std::vector<uchar> imageVector = convertToVector2(posts[i].image, strlen((char *)posts[i].image));
                    std::vector<uchar> binaryData = transformData2(imageVector);

                    json postJson;
                    postJson["id"] = posts[i].id;
                    postJson["title"] = posts[i].userId;
                    postJson["image"] = binaryData;
                    postJson["description"] = posts[i].description;
                    postJson["userName"] = posts[i].userName;
                    postJson["likeCount"] = posts[i].likeCount;
                    postJson["liked"] = posts[i].liked;
                    postsJson.push_back(postJson);
                }
                // Send JSON response
                response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
                response.send(Http::Code::Ok, postsJson.dump());
                delete[] posts; // Free memory allocated for posts
            }
            else
            {
                response.send(Http::Code::Internal_Server_Error, "Failed to fetch posts\n");
            }
        }
        else if (request.resource() == "/filter1" && request.method() == Http::Method::Post)
        {
            auto body = request.body();
            auto jsonBody = json::parse(body);

            std::string imageData = jsonBody["image"].get<std::string>();
            printf("%s", imageData);
            std::vector<unsigned char> decodedData = base64_decode(imageData);
            unsigned char *negativeImageData = applyNegative(decodedData.data(), decodedData.size());
            size_t negativeImageSize = cv::imdecode(decodedData, cv::IMREAD_COLOR).total() * cv::imdecode(decodedData, cv::IMREAD_COLOR).elemSize();
            std::string encodedNegativeImage = base64_encode(negativeImageData, negativeImageSize);
            delete[] negativeImageData;

            json responseBody;
            responseBody["image"] = encodedNegativeImage;

            response.send(Http::Code::Ok, responseBody.dump());
        }
        else
        {
            response.send(Http::Code::Not_Found, "Not Found\n");
        }
    }

private:
    PGconn *conn;
};

extern "C" int functie()
{
    Address addr(Ipv4::any(), Port(9080));
    auto opts = Http::Endpoint::options().threads(1).maxRequestSize(1024 * 1024 * 24);
    Http::Endpoint server(addr);
    server.init(opts);
    server.setHandler(Http::make_handler<HelloHandler>());
    server.serve();
    return 0;
}