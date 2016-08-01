#include <yandex/maps/yacare.h>
#include <yandex/maps/pgpool3/pgpool3.h>
#include <yandex/maps/http2.h>

#include <cairo.h>

#include <boost/optional.hpp>

#include <chrono>
#include <iostream>
#include <sstream>

namespace mpgp = maps::pgpool3;
namespace http2 = maps::http2;

std::unique_ptr<mpgp::Pool> g_poolPtr;

class PgpoolLogger: public mpgp::logging::Logger
{
public:
    PgpoolLogger()
    { }

    virtual void put(mpgp::logging::Message&& msg) override
    {
        std::cout << msg.source << " " << msg.message << std::endl;
    }

    virtual mpgp::logging::Level level() const override
    {
        return mpgp::logging::Level::Info;
    }

    virtual std::string name() const override
    {
        return "pgppool3";
    }
};


template <class T>
boost::optional<T>
getOptionalParam(const yacare::Request& r, const std::string& name)
{
    std::string strParam = r.input()[name];
    if (strParam.empty()) {
        return boost::none;
    }
    try {
        return boost::lexical_cast<T>(strParam);
    } catch (const boost::bad_lexical_cast&) {
        throw yacare::errors::BadRequest() << " Invalid parameter: " << name << " : " << strParam;
    }
}

cairo_surface_t* surface;

void doSomething()
{
    unsigned char* data = cairo_image_surface_get_data(surface);
    int width = cairo_image_surface_get_width (surface);
    int height = cairo_image_surface_get_height (surface);
    for (int i = 0; i < width * height; ++i) {
        unsigned char* offset = data + 4 * i;
        if (
            (int(*(offset + 0)) == 255) and 
            (int(*(offset + 1)) == 255) and
            (int(*(offset + 2)) == 255)
        ) {
            continue;
        }
        if (int(*(offset + 3)) == 255) {
            *(offset + 0) = '\0';
            *(offset + 1) = '\0';           
        }
    }
}



yacare::ThreadPool heavyPool(/* name = */ "heavy", /* threads  = */ 32, /* backlog = */ 16);

YCR_RESPOND_TO("sample:/signals-renderer", YCR_IN_POOL(heavyPool))
{
    auto xOpt = getOptionalParam<uint64_t>(request, "x");
    auto yOpt = getOptionalParam<uint64_t>(request, "y");
    auto zOpt = getOptionalParam<uint64_t>(request, "z");
    if (not xOpt or not yOpt or not zOpt) {
        throw yacare::errors::BadRequest();
    }

    auto connection = g_poolPtr->getMasterConnection();
    auto transaction = mpgp::makeWriteableTransaction(std::move(connection));
    std::stringstream query;
    // query << "select mds_key from signals.signals_tiles where style = 'point'"
    query << "select mds_key from signals_tiles where style = 'point'"
        << " and x = " << *xOpt
        << " and y = " << *yOpt
        << " and z = " << *zOpt
        << ";";
    auto postgresQueryBegin = std::chrono::system_clock::now();
    auto result = transaction->exec(query.str());
    auto postgresQueryEnd = std::chrono::system_clock::now();
    std::chrono::duration<double> postgresQueryDuration = postgresQueryEnd - postgresQueryBegin;

    if (result.empty()) {
        std::cout << "postgres time " << postgresQueryDuration.count() << "sec" << std::endl;
        throw yacare::errors::NotFound() << "no db record";
    }

// GET /signals-renderer?x=74533&y=45031&z=17

    auto mdsKey = result[0][0];

    std::stringstream urlStr;
    // urlStr << "http://storage-int.mdst.yandex.net:80/get-mapsfactory_signals/" << mdsKey;
    // urlStr << "http://storage-int.mds.yandex.net:80/get-mapsfactory_signals/" << mdsKey;
    urlStr << "http://target056i.load.yandex.net:17083/get-mapsfactory_signals/" << mdsKey;

    const http2::URL url{urlStr.str()};
    http2::Client httpClient;
    httpClient.setConnectMethod(http2::connect_methods::withTimeout(
        std::chrono::milliseconds(5000)));
    http2::Request mdsRequest(httpClient, "GET", url);
    auto mdsRequestBegin = std::chrono::system_clock::now();
    auto mdsResponse = mdsRequest.perform();
    auto mdsRequestEnd = std::chrono::system_clock::now();
    std::chrono::duration<double> mdsRequestDuration = mdsRequestEnd - mdsRequestBegin;
    
    if (mdsResponse.status() != 200) {
        throw yacare::errors::InternalError() << "mds returns not 200";
    }

    auto imageBegin = std::chrono::system_clock::now();
    doSomething();
    auto imageEnd = std::chrono::system_clock::now();
    std::chrono::duration<double> imageDuration = imageEnd - imageBegin;

    std::cout << "postgres time " << postgresQueryDuration.count() << "sec; "
        << "mds time " << mdsRequestDuration.count() << "sec; "
        << "image time " << imageDuration.count() << "sec" << std::endl;

    response["Content-Type"] = "image/png";
    response << mdsResponse.body().rdbuf();
    // response << mdsKey;
}

int main(int /*argc*/, const char** /*argv*/)
{

    mpgp::PoolConfigurationPtr poolConfiguration(mpgp::PoolConfiguration::create());

    poolConfiguration->master = mpgp::InstanceId{"target056i.load.yandex.net", 5432};
    // poolConfiguration->master = mpgp::InstanceId{"bko-pgm.tst.maps.yandex.ru", 5432};
    // poolConfiguration->master = mpgp::InstanceId{"gpsdata-pgm.maps.yandex.ru", 5432};
    mpgp::PoolConstantsPtr poolConstants(mpgp::PoolConstants::create(32, 32, 0, 0));
    poolConstants->getTimeoutMs = 5000;
    poolConstants->pingIntervalMs = 5000;

    PgpoolLogger logger;
    g_poolPtr.reset(new mpgp::Pool(
        logger,
        std::move(poolConfiguration),
        "dbname=test user=mapadmin password=mapadmin",
        // "dbname=mapsfactory_signals user=mapsfactory password=V0j1TgArE",
        std::move(poolConstants)
    ));

    INFO() << "Initializing";
    surface = cairo_image_surface_create_from_png("test.png");
    yacare::run();
    cairo_surface_destroy(surface);
    INFO() << "Shutting down";
    return 0;
}