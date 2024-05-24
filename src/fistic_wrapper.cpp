#include <pistache/endpoint.h>
#include <pistache/router.h>
#include "connection.h"
#include <libpq-fe.h>
#include <database.h>
#include <nlohmann/json.hpp>
#include <opencv2/core/hal/interface.h>

using json = nlohmann::json;
using namespace Pistache;

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
        struct Post *posts = nullptr;
        struct User user;
        if (!conn)
        {
            response.send(Http::Code::Service_Unavailable, "Database connection failed\n");
            return;
        }

        if (request.resource() == "/")
        {
            response.send(Http::Code::Ok, "Hello, World\n");
        }
        else if (request.resource() == "/users")
        {
            posts = get_all_posts(conn, 2);
            if (posts)
            {
                json postsJson;
                for (int i = 0; posts[i].id != 0; ++i)
                {
                    printf("%s", posts[i].description);
                    std::vector<uchar> imageVector = convertToVector2(posts[i].image, strlen((char*)posts[i].image));
                    std::vector<uchar> binaryData = transformData2(imageVector);

                    json postJson;
                    postJson["id"] = posts[i].id;
                    postJson["title"] = posts[i].userId;
                    postJson["image2"] = binaryData;
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
    auto opts = Http::Endpoint::options().threads(1);
    Http::Endpoint server(addr);
    server.init(opts);
    server.setHandler(Http::make_handler<HelloHandler>());
    server.serve();
    return 0;
}