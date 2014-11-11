#include <yandex/maps/sprav/model/storages/revision.h>
#include <yandex/maps/sprav/model/convert.h>
#include <yandex/maps/sprav/common/config.h>
#include <yandex/maps/sprav/common/stopwatch.h>
#include <yandex/maps/sprav/algorithms/datamodel/company.h>
#include <yandex/maps/sprav/algorithms/similarity/similarity.h>
#include <yandex/maps/sprav/algorithms/unification/email_unifier.h>
#include <yandex/maps/sprav/algorithms/unification/url_unifier.h>
#include <yandex/maps/sprav/algorithms/unification/address_unifier.h>

#include <yandex/maps/pgpool2/pgpool.h>
#include <yandex/maps/common/exception.h>

#include <boost/lexical_cast.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>


namespace sprav = maps::sprav;
namespace model = maps::sprav::model;
namespace algorithms = maps::sprav::algorithms;


struct ReadSessionHolder {
    ReadSessionHolder(maps::pgpool2::ConnectionMgr& connMgr) :
        transaction(connMgr.readTransaction()),
        storage(new model::RevisionStorage(*transaction)),
        session(storage, model::Session::CachingPolicy::CacheAll)
    {}

    maps::pgpool2::PgTransaction transaction;
    std::shared_ptr<model::RevisionStorage> storage;
    model::Session session;
};


int main()
{
    std::cout << "hello" << std::endl;

    auto connectionManager =
        sprav::config::createConnectionManager(
            sprav::config::config(),
            sprav::config::ConnectionPoolProfile::Editor);
    ReadSessionHolder holder(*connectionManager);

    std::vector<model::search::Filter> filters;
    filters.push_back(model::search::CompanyUrl.equals("http://www.obrbank.ru"));
    auto ids = holder.session.loadCompanyIds(filters);
    for (auto id: ids) {
        std::cout << "id = " << id << std::endl;
    }
    return 0;
}