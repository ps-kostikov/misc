#!/usr/bin/python

import sys
import apt
import requests

cache = apt.Cache()

def hosts(package_name):
    resp = requests.get('https://c.yandex-team.ru/api/package2hosts/{0}'.format(
        package_name))
    return resp.text.split()


def is_package_important(package_name):
    resp = requests.get('https://c.yandex-team.ru/api/package_version/{0}'.format(
        package_name))
    j = resp.json()
    if package_name not in j:
        return False
    return j[package_name]['stable']['version'] is not None


def depends(package_name):
    if package_name not in cache:
        return []
    package = cache[package_name]
    # get last version
    version = package.versions[0]
    result = []
    for dependency in version.dependencies:
        for base_dependency in dependency.or_dependencies:
            if base_dependency.rawtype != 'Depends':
                continue
            result.append(base_dependency.name)
    return result


def all_depends(package_name):
    result = []
    current = [package_name]
    nexts = []
    handled = []
    while current:
        for pn in set(current):
            deps = depends(pn)
            handled.append(pn)

            for d in deps:
                if d not in handled:
                    nexts.append(d)

            result.extend(deps)

        current = nexts
        nexts = []
    return list(set(result))


print('hello')

package_name = sys.argv[1]

# print(package_name)
# print(hosts(package_name))
# print(depends(package_name))

# print(is_package_important(package_name))
# print(is_package_important('libboost-program-options1.40.0'))

# all_d = all_depends(package_name)
# print(all_d)
# print(filter(lambda pn: is_package_important(pn), all_d))

p_hosts = hosts(package_name)

for dep in all_depends(package_name):
    if not is_package_important(dep):
        continue
    if not 'yandex' in dep:
        continue

    d_hosts = hosts(dep)
    missed_hosts = [h for h in p_hosts if h not in d_hosts]
    if missed_hosts:
        print("{0} has missed hosts {1}".format(dep, missed_hosts))
