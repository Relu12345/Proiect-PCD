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
#include "base64.h" // Include the base64 library
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

using json = nlohmann::json;
using namespace Pistache;

struct ImageData {
    unsigned char* dataPtr;
    size_t dataSize;
};

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
        if (!conn || PQstatus(conn) != CONNECTION_OK)
        {
            response.send(Http::Code::Service_Unavailable, "Database connection failed\n");
            return;
        }

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
        if (request.resource() == "/filter1" && request.method() == Http::Method::Post) {
            auto body = request.body();
            auto jsonBody = json::parse(body);

            std::vector<uint8_t> imageData = jsonBody["data"].get<std::vector<uint8_t>>();
            int width = jsonBody["width"];
            int height = jsonBody["height"];

            // Print imageData content
            std::cout << "Received image data (" << imageData.size() << " bytes):\n";
            for (size_t i = 0; i < imageData.size(); ++i) {
                std::cout << std::setw(3) << static_cast<int>(imageData[i]) << " ";
                if ((i + 1) % 20 == 0) {  // Print 20 values per line for readability
                    std::cout << "\n";
                }
            }
            std::cout << "\n";

            // Convert imageData to cv::Mat
            cv::Mat image(height, width, CV_8UC4, imageData.data());  // Assuming RGBA

            // Convert to grayscale
            cv::Mat grayImage;
            cv::cvtColor(image, grayImage, cv::COLOR_RGBA2GRAY);

            // Convert to black and white
            cv::Mat bwImage;
            cv::threshold(grayImage, bwImage, 128, 255, cv::THRESH_BINARY);

            // Encode the result back to a string
            std::vector<uchar> buf;
            cv::imencode(".jpg", bwImage, buf);

            json responseBody;
            responseBody["image"] = std::vector<char>(buf.begin(), buf.end());

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