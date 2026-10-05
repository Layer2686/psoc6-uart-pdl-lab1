# PSoC 6 UART на PDL

Приклад передачі та приймання пакетів через UART на **Infineon CY8CPROTO-062-4343W**.
Реалізація використовує **PDL** і апаратні FIFO, без HAL, `retarget-io`, `printf`,
DMA та UART-переривань. Проєкт підготовлено для лабораторної роботи №1,
варіант 8: **38400 baud, 8N1**, тестове повідомлення **LAZOR**.

## Плата та інтерфейс

| Параметр | Значення |
|---|---|
| Плата | CY8CPROTO-062-4343W |
| Мікроконтролер | CY8C624ABZI-S2D44, програма на Cortex-M4 |
| Програматор / USB-UART | Вбудований KitProg3 |
| UART | SCB5 |
| TX плати | P5.1, підключений до RX KitProg3 |
| RX плати | P5.0, підключений до TX KitProg3 |
| Baudrate | 38400 |
| Формат | 8N1: 8 біт даних, без парності, 1 стоп-біт |
| Flow control | None |

Зв'язок із ПК проходить через KitProg3. З'єднувати TX і RX плати між собою
не потрібно. На пінах використовуються GPIO-рівні UART; для фізичного
інтерфейсу RS-232 з DB9 потрібен окремий перетворювач рівнів.

## Протокол

```text
ПК -> плата:  LAZOR\r
Плата -> ПК:  ACK:LAZOR\n
```

Запит завершується **CR** (`\r`, 0x0D), відповідь — **LF** (`\n`, 0x0A).
Плата повертає `ACK:` і фактично прийнятий текст. Допускається до 63
друкованих ASCII-символів у запиті. Пошкоджені або задовгі пакети отримують
`ERR:INVALID_PACKET\n`.

Після Reset виводяться `LAZOR`, параметри UART та підказка.
Відповідь надсилається після завершення пакета; побайтового echo немає.

## Вимоги

- Git.
- ModusToolbox 3.9 із GCC ARM та ModusToolbox Programming Tools / OpenOCD.
- Драйвер KitProg3.
- VS Code з розширенням ModusToolbox; Cortex-Debug потрібен для відлагодження.
- UART-термінал або PowerShell для автоматичної перевірки.

Перевірено збірку Debug і Release у Windows із ModusToolbox tools 3.9,
GCC ARM 14.2.1 та PDL 3.8.0. Інші середовища не перевірялися.

## Перший запуск

У ModusToolbox Shell виконайте:

```sh
git clone https://github.com/Layer2686/psoc6-uart-pdl-lab1.git
cd psoc6-uart-pdl-lab1
make getlibs
```

Репозиторій публічний; для клонування авторизація GitHub не потрібна.
`make getlibs` відновлює зафіксовані залежності у сусідній папці `../mtb_shared`
і генерує службові файли збірки. BSP та конфігурація плати входять до репозиторію.

У VS Code відкрийте файл:

```text
mtb-example-hal-uart-transmit-receive.code-workspace
```

У **User Settings** задайте `modustoolbox.toolsPath` — локальний шлях до
папки `tools_3.9`. Налаштування шляхів виконуються один раз на кожному ПК.
Далі через **Terminal -> Run Task** доступні **Build** та **Build & Program**.

Для відлагодження через Cortex-Debug у User Settings додатково задаються:

- `cortex-debug.armToolchainPath` — папка GCC `bin`;
- `cortex-debug.openocdPath` — шлях до `openocd.exe`;
- `cortex-debug.openocdScriptPath` — папка OpenOCD `scripts`.

## Збірка та прошивка

Підключіть плату USB-кабелем із передачею даних до роз'єму KitProg3.
У VS Code запустіть задачу **Build & Program**.

Ті самі дії через ModusToolbox Shell:

```sh
make build -j4
make program
```

`make program` збирає та прошиває програму через KitProg3.
Для вже зібраної прошивки доступна команда `make qprogram`.
Файл прошивки:

```text
build/APP_CY8CPROTO-062-4343W/Debug/mtb-example-hal-uart-transmit-receive.hex
```

Назва файлу успадкована від початкового прикладу Infineon.

## Перевірка UART

1. У терміналі оберіть порт **KitProg3 USB-UART**. Його номер можна знайти
   в Диспетчері пристроїв Windows; номер COM залежить від комп'ютера.
2. Встановіть **38400, 8N1, Flow control: None**.
3. Натисніть **Reset** на платі: з'явиться стартове повідомлення.
4. Надішліть `LAZOR`, завершивши рядок CR або CR+LF. Очікувана відповідь:
   **`ACK:LAZOR`**. Для відображення введення можна ввімкнути local echo.

Для автоматичної перевірки закрийте термінал, щоб звільнити порт, і запустіть
із папки проєкту у PowerShell. Замініть `COM4` на номер порту плати:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\host\uart_host.ps1 -Port COM4 -Message LAZOR -Repeat 3
```

Скрипт використовує стандартний `System.IO.Ports.SerialPort`, надсилає запит,
перевіряє повну відповідь та виводить hex-дампи. Додаткові пакети не потрібні.
`ExecutionPolicy Bypass` діє лише для запущеного процесу PowerShell.
Під час обміну не слід натискати Reset, щоб стартовий текст не потрапив у відповідь.

Приклад очікуваного результату:

```text
TX bytes (6): 4C 41 5A 4F 52 0D
RX text: 'ACK:LAZOR'
RX bytes (10): 41 43 4B 3A 4C 41 5A 4F 52 0A
Verify: OK
```

Перевірка формування пакета без плати:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\host\uart_host.ps1 -Loopback
```

Software loopback використовує потік у пам'яті та не перевіряє UART або ACK.

## Оновлення наявного клону

```sh
git pull --ff-only
```

Після оновлення відкрийте workspace у VS Code. Якщо інструменти налаштовані
та залежності вже завантажені, додаткове налаштування не потрібне.
Для встановлення нової версії прошивки запустіть **Build & Program**.

Якщо в оновленні змінилися залежності, перед збіркою повторіть `make getlibs`.
Ця команда також обов'язкова після першого клонування на новому ПК.

## Нестандартне розташування інструментів

У ModusToolbox Shell шлях можна задати для поточної сесії:

```sh
export CY_TOOLS_PATHS='C:/path/to/ModusToolbox/tools_3.9'
make getlibs
make build -j4
```

Якщо GCC не визначається автоматично:

```sh
make build -j4 CY_COMPILER_GCC_ARM_DIR='C:/path/to/gcc/bin'
```

## Структура проєкту

- `main.c` — приймання пакетів до CR та формування ACK.
- `uart_pdl.c`, `uart_pdl.h` — налаштування SCB5, тактового дільника та RX/TX FIFO.
- `host/uart_host.ps1` — передавач і перевірка відповіді на ПК.
- `bsps/TARGET_APP_CY8CPROTO-062-4343W/` — BSP і конфігурація плати.
- `deps/`, `libs/*.mtb` — дескриптори залежностей.
- [LAB1.md](LAB1.md) — пояснення алгоритму, розрахунки та параметри лабораторної.

## Ліцензія

Проєкт базується на прикладі Infineon. Умови використання початкового коду
наведено в `LICENSE`, заголовках файлів та `LICENSE`/`EULA` каталогу BSP.
