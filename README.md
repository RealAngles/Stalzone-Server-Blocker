# Stalzone Server Blocker

> Побег с msk2 теперь и от github!



## Скриншот

<img width="637" height="615" alt="image" src="https://github.com/user-attachments/assets/56359b09-8f19-41c4-bd37-24386a162630" />


---

## Исправление ошибки VCRUNTIME

Установите пак

All in one c++ - https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/

---

## Сборка

Для самостоятельной сборки потребуется:

- **Visual Studio 2022** (рекомендуется)
- Windows SDK
- C++ Desktop Development

Также необходимы используемые библиотеки проекта:

- Dear ImGui
- nlohmann/json
- WinDivert
- DirectX 11

После открытия решения в Visual Studio достаточно собрать проект в режиме **Release x64**.

---

## Готовая сборка

Если не хотите собирать самостоятельно, скачайте готовую версию:

https://github.com/RealAngles/Stalzone-Server-Blocker/releases/tag/Release

---

## Как работает

Программа получает актуальный список серверов, после чего позволяет выбрать нужные.

После нажатия **«Заблокировать»** создаётся фильтр WinDivert, который блокирует сетевой трафик к выбранным игровым серверам STALZONE.

Настройки автоматически сохраняются в `Settings.json`.

---

## Используемые технологии

- C++
- WinAPI
- Dear ImGui
- DirectX 11
- WinHTTP
- WinDivert
- nlohmann/json

---

## Основано на

Проект использует библиотеку:

**unofficial-stalzone-api**

https://github.com/Art3mLapa/unofficial-stalzone-api

Спасибо **Art3mLapa** за проделанную работу.

---

# Credits

## YungDaggerStab

YouTube

https://youtube.com/@PoshelNaxuy

Telegram

https://t.me/RealAngles

Telegram Channel

https://t.me/BestGook


## WeedSellerBand

Telegram

https://t.me/ker9j

---

## Art3mLapa

Автор библиотеки **unofficial-stalzone-api**

GitHub

https://github.com/Art3mLapa

Telegram

https://t.me/bscp_podval

---
