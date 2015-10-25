#!/usr/bin/python

import requests
import json
import time


TOKEN = '158722483:AAHOZ41kr3Weag5sYBc_etxIKvyeMs3s_qQ'
BASE_URL = 'https://api.telegram.org/bot' + TOKEN + '/'


while True:
    updates = requests.get(BASE_URL + 'getUpdates').json()['result']
    # resp = requests.get(BASE_URL + 'getMe').json()

    chat_ids = set()
    for update in updates:
        message = update.get('message')
        if not message:
            continue
        chat = message.get('chat')
        if not chat:
            continue
        chat_id = chat.get('id')
        if chat_id:
            chat_ids.add(chat_id)

    print(chat_ids)

    for chat_id in chat_ids:
        url = BASE_URL + 'sendMessage' + '?' + 'chat_id=' + str(chat_id) + '&' + 'text=' + 'qua'
        message = requests.get(url).json()
        print(
            json.dumps(message,
                sort_keys=True, indent=4, separators=(',', ': '))
        )

    time.sleep(10)
