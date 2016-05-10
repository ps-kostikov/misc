# curl -X POST \
#     -u c5d5c3b85ee6484fbdabd5b04324e545: \
#     -d args='{"var1":"value1"}' \
#     -d async=false \
#     'https://test.sender.yandex-team.ru/api/0/nmap/transactional/0EDQ2I02-JPA/send?to_email=pkostikov@yandex-team.ru'



import requests

resp = requests.post(
    'https://test.sender.yandex-team.ru/api/0/nmap/transactional/0EDQ2I02-JPA/send?to_email=pkostikov@yandex-team.ru',
    auth=('c5d5c3b85ee6484fbdabd5b04324e545', ''),
    data={
        'async': 'false'
    },
    verify=False
)
print resp.text