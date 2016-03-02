#include <yandex/maps/yacare.h>
#include <yandex/maps/pgpool3/pgpool3.h>
#include <yandex/maps/http2.h>

#include <boost/optional.hpp>

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

    virtual void put(mpgp::logging::Message&& /*msg*/) override
    {
        // std::cout << msg.source << " " << msg.message << std::endl;
    }

    virtual mpgp::logging::Level level() const override
    {
        return mpgp::logging::Level::Debug;
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

yacare::ThreadPool heavyPool(/* name = */ "heavy", /* threads  = */ 16, /* backlog = */ 16);

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
    query << "select mds_key from signals.signals_tiles where style = 'point'"
        << " and x = " << *xOpt
        << " and y = " << *yOpt
        << " and z = " << *zOpt
        << ";";
    auto result = transaction->exec(query.str());

    if (result.empty()) {
        throw yacare::errors::NotFound() << "no db record";
    }

// GET /signals-renderer?x=74533&y=45031&z=17

    auto mdsKey = result[0][0];

    std::stringstream urlStr;
    urlStr << "http://storage-int.mdst.yandex.net:80/get-mapsfactory_signals/" << mdsKey;
    // urlStr << "http://storage-int.mds.yandex.net:80/get-mapsfactory_signals/" << mdsKey;

    const http2::URL url{urlStr.str()};
    http2::Client httpClient;
    httpClient.setConnectMethod(http2::connect_methods::withTimeout(
        std::chrono::milliseconds(5000)));
    http2::Request mdsRequest(httpClient, "GET", url);
    auto mdsResponse = mdsRequest.perform();
    
    if (mdsResponse.status() != 200) {
        throw yacare::errors::InternalError() << "mds returns not 200";
    }
    response["Content-Type"] = "image/png";
    response << mdsResponse.body().rdbuf();
    // response << mdsKey;
}

int main(int /*argc*/, const char** /*argv*/)
{

    mpgp::PoolConfigurationPtr poolConfiguration(mpgp::PoolConfiguration::create());

    poolConfiguration->master = mpgp::InstanceId{"bko-pgm.tst.maps.yandex.ru", 5432};
    // poolConfiguration->master = mpgp::InstanceId{"gpsdata-pgm.maps.yandex.ru", 5432};
    mpgp::PoolConstantsPtr poolConstants(mpgp::PoolConstants::create(16, 16, 0, 0));
    poolConstants->getTimeoutMs = 5000;
    poolConstants->pingIntervalMs = 5000;

    PgpoolLogger logger;
    g_poolPtr.reset(new mpgp::Pool(
        logger,
        std::move(poolConfiguration),
        "dbname=mapsfactory_signals user=mapsfactory password=V0j1TgArE",
        std::move(poolConstants)
    ));

    INFO() << "Initializing";
    yacare::run();
    INFO() << "Shutting down";
    return 0;
}
