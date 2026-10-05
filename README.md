# PSoC 6 UART на PDL — лабораторна 1, варіант 8

Передача та приймання пакетів через UART на **Infineon CY8CPROTO-062-4343W**.
Мікроконтролер: **CY8C624ABZI-S2D44**, програма виконується на **Cortex-M4**.
Драйвер написаний на **PDL**, без HAL, `retarget-io`, `printf`, DMA та UART-переривань.

## Параметри та протокол

| Параметр | Значення |
|---|---|
| Варіант | 8 |
| Baudrate | **38400** |
| Формат кадру | **8N1**: 8 біт даних, без парності, 1 стоп-біт |
| Flow control | **None** |
| Повідомлення | **LAZOR** |
| UART | **SCB5** |
| TX плати | **P5.1**, підключений до RX KitProg3 |
| RX плати | **P5.0**, підключений до TX KitProg3 |
| Програматор / USB-UART | Вбудований **KitProg3** |

```text
ПК → плата:  LAZOR\r
Плата → ПК:  ACK:LAZOR\n
```

CR (`\r`, 0x0D) завершує запит; LF (`\n`, 0x0A) завершує відповідь.
Після Reset плата також виводить `LAZOR`, параметри UART і підказку.
Максимальна довжина запиту — 63 друковані ASCII-символи.
Пошкоджені або задовгі пакети отримують `ERR:INVALID_PACKET\n`.

## Що встановити на ноутбук

- **Git** та доступ до GitHub-акаунта, якому доступний цей приватний репозиторій.
- **ModusToolbox 3.9** з компонентами **GCC ARM** і **ModusToolbox Programming Tools / OpenOCD**.
- USB-драйвер **KitProg3** з інсталяції ModusToolbox.
- За бажанням: VS Code з розширеннями **ModusToolbox** та **Cortex-Debug**.
- UART-термінал, наприклад уже встановлений на твоєму ПК, або PowerShell для скрипта перевірки.

Перевірене середовище: Windows, ModusToolbox tools 3.9, GCC ARM 14.2.1,
PDL 3.8.0. Debug і Release збираються. Живий обмін цією версією прошивки
ще потрібно перевірити на платі.

## Клонування та залежності

Відкрий **ModusToolbox Shell** з меню Windows: у ньому доступні `make`, `git`
та інструменти збірки. Клонуй у каталог без пробілів у шляху:

```sh
git clone https://github.com/Layer2686/psoc6-uart-pdl-lab1.git
cd psoc6-uart-pdl-lab1
make getlibs
make build -j4
```

Для приватного репозиторію GitHub може запросити вхід через браузер.
Потрібен той самий акаунт або наданий власником доступ.

`make getlibs` завантажує бібліотеки Infineon у **сусідню папку `../mtb_shared`**
і створює службові файли `libs/mtb.mk`. Версії залежностей зафіксовані в
`deps/`, `libs/*.mtb`, BSP-дескрипторах та `deps/assetlocks.json`.
У репозиторії є наш BSP і згенерована конфігурація плати; копіювати
`mtb_shared` або `build` зі старого ПК не потрібно.

Якщо інструменти встановлені не у стандартний каталог, задай шлях до
`tools_3.9` у цій сесії ModusToolbox Shell, використовуючи прямі слеші:

```sh
export CY_TOOLS_PATHS='C:/your/path/ModusToolbox/tools_3.9'
make getlibs
make build -j4
```

Якщо GCC встановлений окремо і не визначається автоматично:

```sh
make build -j4 CY_COMPILER_GCC_ARM_DIR='C:/your/path/gcc/bin'
```

## Як прошити плату

1. Підключи USB-кабель з передачею даних до роз'єму плати, підключеного до **KitProg3**.
2. У ModusToolbox Shell, перебуваючи в папці проєкту, виконай:

   ```sh
   make program
   ```

   Команда збирає програму та прошиває її через KitProg3. Для вже зібраної
   прошивки можна використати `make qprogram`.
3. Відкрий UART-термінал і встанови **38400, 8N1, Flow control: None**.
   Обери порт із назвою **KitProg3 USB-UART** у Диспетчері пристроїв.
   На початковому ПК це **COM4**; на ноутбуці номер може бути іншим.
4. Натисни **Reset** на платі — з'явиться `LAZOR` і підказка.
5. Введи `LAZOR` та відправ CR або CR+LF. Відповідь: **`ACK:LAZOR`**.

Програма не повертає кожну літеру під час введення: вона відповідає після
завершення всього пакета. Для видимості введення можна ввімкнути **local echo**.
З'єднувати P5.0 і P5.1 між собою не потрібно: зв'язок із ПК уже проходить через
KitProg3. Це GPIO UART, а не біполярні рівні фізичного RS-232 на DB9.

Результат збірки збережений тут (назва успадкована від початкового прикладу):

```text
build/APP_CY8CPROTO-062-4343W/Debug/mtb-example-hal-uart-transmit-receive.hex
```

## Автоматична перевірка обміну

Закрий UART-термінал, щоб звільнити COM-порт. У **PowerShell** з папки проєкту:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\host\uart_host.ps1 -Port COM4 -Message LAZOR -Repeat 3
```

На ноутбуці заміни `COM4` на його порт KitProg3. Скрипт задає 38400/8N1,
надсилає `LAZOR\r`, перевіряє повну відповідь `ACK:LAZOR\n` і друкує hex-дампи
та `Verify: OK`. Під час перевірки не натискай Reset: стартовий текст порушить
очікуваний формат відповіді. Додаткові пакети встановлювати не потрібно.
`ExecutionPolicy Bypass` діє лише для цього процесу PowerShell.

Очікувані дані при успішному обміні (це приклад, не журнал вимірювання):

```text
TX bytes (6): 4C 41 5A 4F 52 0D
RX text: 'ACK:LAZOR'
RX bytes (10): 41 43 4B 3A 4C 41 5A 4F 52 0A
Verify: OK
```

Без плати можна перевірити тільки формування пакета:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\host\uart_host.ps1 -Loopback
```

Software loopback використовує потік у пам'яті; він не перевіряє UART плати чи ACK.

## VS Code

Відкрий `mtb-example-hal-uart-transmit-receive.code-workspace`.
У **User Settings** VS Code задай локальний `modustoolbox.toolsPath` — шлях до
своєї папки `tools_3.9`. Через `Terminal → Run Task` доступні **Build** і
**Build & Program**. Конкретні шляхи інструментів цього ПК не збережені у Git.

Для відлагодження через Cortex-Debug додатково задай у User Settings:

- `cortex-debug.armToolchainPath`: локальна папка GCC `bin`;
- `cortex-debug.openocdPath`: локальний `openocd.exe`;
- `cortex-debug.openocdScriptPath`: локальна папка OpenOCD `scripts`.

За потреби IDE-конфігурацію можна перегенерувати командою `make vscode`.

## Оновлення на ноутбуці

```sh
git pull --ff-only
make getlibs
make build -j4
make program
```

`make getlibs` потрібний після першого клонування і при зміні залежностей.
Якщо після оновлення змінилася конфігурація збірки та виникли проблеми
зі старим кешем, виконай `make clean`, потім `make build -j4`.

## Файли та пояснення

- `main.c` — накопичення запиту до CR і формування ACK.
- `uart_pdl.c`, `uart_pdl.h` — налаштування SCB5, тактового дільника, RX/TX FIFO.
- `host/uart_host.ps1` — передавач і перевірка відповіді на ПК.
- `bsps/TARGET_APP_CY8CPROTO-062-4343W/` — конфігурація саме цієї плати.
- [LAB1.md](LAB1.md) — докладне пояснення алгоритму, дільника та часових параметрів.

Для варіанта 8 значення 200 мс, датчик BME280/BMP180 та Mock USB Logitech
із таблиці належать лабораторним 5, 4 та 3 відповідно. Ця програма реалізує
лабораторну 1, тому не додає періодичної передачі або датчика.

## Ліцензія

Проєкт походить від прикладу Infineon. Умови використання початкового коду
містяться в `LICENSE`, у заголовках файлів та в `LICENSE`/`EULA` каталогу BSP.
Наявність репозиторію не змінює цих умов.
