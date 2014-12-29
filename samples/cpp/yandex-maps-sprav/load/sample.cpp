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

#include <boost153/lexical_cast.hpp>

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


std::vector<model::ID>
readIds(const std::string& filename)
{
    std::ifstream is(filename.c_str());
    std::string line;
    std::vector<model::ID> result;

    while(std::getline(is, line)) {
        auto id = boost153::lexical_cast<model::ID>(line);
        result.push_back(id);
    }
    return result;
}


int main(int /*argc*/, char* argv[])
{
    std::cout << "hello" << std::endl;
    std::string idsFilename = std::string(argv[1]);
    std::cout << "read " << idsFilename << std::endl;
    std::vector<model::ID> ids = readIds(idsFilename);

    auto connectionManager =
        sprav::config::createConnectionManager(
            sprav::config::config(),
            sprav::config::ConnectionPoolProfile::Editor);
    ReadSessionHolder holder(*connectionManager);

    // std::vector<model::Company> companies;
    // for (auto id: ids) {
    //     companies.push_back(holder.session.loadCompany(id));
    // }
    auto companies = holder.session.loadCompany(ids);
    std::cout << "size of result: " << companies.size() << std::endl;
    return 0;
}