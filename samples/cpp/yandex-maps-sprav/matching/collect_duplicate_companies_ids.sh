#!/bin/bash -x

echo 'select id from view.companies where publishing_status = 4;' | psql -tA 'host=draco.backa.dev.yandex.net port=5432 user=mapadmin password=mapadmin dbname=sprav_master' > duplicate_companies_ids