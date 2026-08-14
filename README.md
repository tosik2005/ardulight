# WS2811 white LED strip and radar control project

## 1. General description


The prodject is designed to control a WS2811-based LED strip using an Arduino Mega 2560 R3 with subsequent radar integration.

The strip uses white LEDs with a color temperature of 6000K. Control is not done as a classic RGB-color strips, but as brithness channels for individual LED blocks within a pixel.

---

## 2. Project status

|      Section      |         Status      |                         Note                     |
|-------------------|---------------------|--------------------------------------------------|
| Hardware part     |        at work      | there are 2 ws2811 pixels on the layout          |
| Software part     |        at work      | Arduino Mega 2560 R3                             |
| Radars            |        at work      | setting gate levels via USB-TTL adapter          |
| README            | constantly updated  | updated based on confirmed facts                 |

---

## 3. Components used
-- Arduino Mega 2560 R3
-- led strip based on all WS2811 chips 24V
-- presence sensor HLK-LD2410C-P
-- DC power unit 24 V
-- DC-DC step-down controller LM2596
-- 8-channel logic level equalizer
-- super diode
-- other electrical components

---

## 4. Single pixel structure

One WS2811 pixel controls 30 white LEDs.

Inside the pixel, the LEDs are divided into 3 blocks of 10 LEDs each:

|channel WS2811|       block of LEDs      |              Appointment            |
|--------------|--------------------------|-------------------------------------|
|       R      | first 10 LEDs            | brightness control first block      |
|       G      | second 10 LEDs           | brightness control first block      |
|       B      | last 10 LEDs             | brightness control first block      |

Since the LEDs are white, the R/G/B values are not considered as color, but as brightness levels of the corresponding blocks

---

## 5. Топологія підключення пікселів

На макеті використовуються 2 пікселі.

Можливі два варіанти підключення:

### 5.1. Послідовне підключення

Використовується для послідовної адресції пікселів.

Очікувана логіка нумерації:

- перший піксель: індекс 0;
- другий піксель: індекс 1;
- далі за потреби: 2, 3, ..., n.

### 5.2. Паралельне підключення

Використовується для паралельної роботи з двома пікселями одночасно.

пікселі отримують однакові дані;

---

## 6. Апаратна гілка

Пов’язана розмова/гілка:

- «Апаратна реалізація світлодіодної стрічки».

У цьому розділі README фіксуються:

- фізичне підключення;
- живлення;
- data-лінії;
- кількість пікселів;
- структура блоків світлодіодів;
- обмеження струму;
- особливості WS2811;
- перевірка на макеті.

---

## 7. Програмна гілка

Пов’язана розмова/гілка:

- «Arduino Mega R3 2560 programming».

У цьому розділі README фіксуються:

- логіка керування пікселями;
- поточна поведінка світлодіодів;
- використані бібліотеки або власні функції, якщо вони підтверджені;
- діагностика;
- логи;
- інтеграція з радарами.

---

## 8. Керування пікселями

Поточна цільова модель керування:

- один піксель містить 3 блоки по 10 світлодіодів;
- кожен блок керується окремо через відповідний канал WS2811;
- для білих світлодіодів керування зводиться до зміни яскравості блоків.

Статус реалізації:

- базова адресація пікселів: потребує підтвердження;
- незалежне керування пікселями: потребує підтвердження;
- керування блоками всередині пікселя: потребує підтвердження.

---

## 9. Радари

Розділ буде заповнений після отримання фактичних даних.

Потрібно зафіксувати:

- тип радара;
- інтерфейс підключення;
- формат даних;
- події, які має обробляти Arduino;
- реакцію світлодіодів на події радара.

Статус:

- інтеграція: не почата або очікує вхідних даних.

---

## 10. Правила ведення README

1. README оновлюється лише за підтвердженими фактами.
2. Не описуються події або функції, які ще не перевірені.
3. Якщо даних недостатньо — використовується статус `TBD` або `потребує підтвердження`.
4. Зміни вносяться окремо для апаратної та програмної частин.
5. Код у README додається лише за потреби і тільки як підтверджений фрагмент.
6. README має відображати поточний реальний стан проєкту.
