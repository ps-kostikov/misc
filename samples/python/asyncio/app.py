from aiohttp import web

import logging

logging.basicConfig(format='%(asctime)s %(levelname)s:%(message)s', level=logging.DEBUG)


async def index(request):
    return web.Response(text="Welcome home!")


async def my_web_app():
    for i in range(100000000): j=i
    logging.info("long operation before app created")
    app = web.Application()
    app.router.add_get('/', index)
    return app
