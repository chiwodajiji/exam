# 怠惰終結器（NUCLEO-G474RE 期中專題）

---

## 1. 情境故事：誰、在哪、遇到什麼麻煩

**誰**：常拖延、總覺得「等一下再開始」的人——也就是大多數的我們。

**在哪**：書桌前，該開始寫報告、讀書、打掃，卻一直滑手機、發呆。

**遇到什麼麻煩**：做事最大的成本不是「做」，而是「開始」那一瞬間的惰性；任務越模糊，越不想動。

**裝置怎麼幫忙**：把「先做一件事」切成最小單位——**「就做幾分鐘試試」**。
在電腦上設定幾分鐘、按下開始，三顆燈依序漸暗代表時間在流走；全暗代表「這幾分鐘你撐住了」，
可以停，也可以繼續。中途按 PC9 可暫停休息。用「看得見的時間流走」取代手機計時，
把抽象的「再一下」變成肉眼可見的燈光。

---

## 2. 替身對照：故事裡的模組 → 板上替身

| 故事裡的模組 | 板上替身 | 腳位 / 周邊 | 時間縮放 |
| --- | --- | --- | --- |
| 時間在流走 | 三顆 LED 依序漸暗 | PA0 / PA1 / PB10（TIM2 PWM） | 1:1（現實幾分鐘＝裝置幾分鐘） |
| 「開始做」這個動作 | `SET <分鐘>` + `START` 指令 | LPUART1（PA2/PA3）115200 | — |
| 中間想休息 / 暫停 | PC9 外接按鈕切換 | PC9（EXTI 下降緣） | — |
| 「時間到」的提醒 | `TIME_UP` 字串 | 終端機（LPUART1 TX） | — |
| 真實計時器外殼 | NUCLEO-G474RE 開發板 | 板載 ST-LINK 供電 | — |

> 換成真實裝置時：把 LED 替身換成段顯 / 螢幕，把序列埠換成實體旋鈕，
> 把 `TIME_UP` 換成蜂鳴器即可（見第 10 節）。

---

## 3. 功能與驗收條件：動作、預期、實測結果

| 動作 | 預期結果 | 實測結果（依程式邏輯） | 過 / 不過 |
| --- | --- | --- | --- |
| 上電、開啟終端機 | 板子等待指令，三燈全暗 | state=0，主迴圈持續將 compare 設 0 | 過 |
| 終端機送 `SET 1` | 回 `收到指令: [SET 1]`，進入就緒 | `countdown_time=1`、`state=1` | 過 |
| 終端機送 `START` | 三燈瞬間全亮，開始依序漸暗 | `state=2`，PA0→PA1→PB10 各 200 步 | 過 |
| 等待至三燈全暗 | 回 `TIME_UP`，回到等待 | 送 `TIME_UP\r\n`，`state=0` | 過 |
| 執行中按 PC9 | 暫停 / 繼續（忙等） | `pause_flag` 切換，200ms 軟體消抖 | 過 |
| `START` 但未先 `SET` | 不開始（需 `countdown_time>0`） | 僅在 `countdown_time>0` 才 `state=2` | 過 |

---

## 4. 用到的技術：第 3～7 週勾選表（同投影片）

- [x] **GPIO 輸出**：TIM2 PWM 驅動三顆 LED（PA0/PA1/PB10）
- [x] **GPIO 輸入**：PC9 外接暫停按鈕（EXTI 來源腳）
- [x] **複用功能 AF**：LPUART1 接在 PA2/PA3（MODER=10，AFR 選 LPUART1）
- [x] **時脈 PLL 170MHz**：HSI16 ÷4 ×85 ÷2，FLASH 4 等待週期 + boost
- [x] **外部中斷 EXTI**：PC9 下降緣觸發暫停（EXTI9_5）
- [x] **NVIC 致能**：EXTI9_5 IRQ 進得來
- [x] **優先權與巢狀**：EXTI9_5 priority 0
- [x] **軟體去彈跳**：時間戳 `ticks − t_btn ≥ 200ms` 才算
- [x] **UART 傳送**：LPUART1 115200 8N1，回顯與 `TIME_UP`
- [x] **UART 接收＋環形緩衝**：RX 中斷逐 byte 收集（30 bytes 緩衝），遇換行才解析
- [ ] **Timer 定時中斷**：沒用到（本專題用 PWM 非定時中斷計時）
- [x] **PWM**：TIM2 CH1/2/3，Prescaler=169、Period=999（1ms PWM）
- [ ] **輸入捕捉**：沒用到

> 本專題用到 **11 項**、沒用 **2 項**（Timer 定時中斷、輸入捕捉）。打勾的每一項都指得出程式在哪。

---

## 5. 腳位表：用到的每支腳與用途

| 腳位 | 訊號 / 標籤 | 用途 | 方向 | 備註 |
| --- | --- | --- | --- | --- |
| PA0 | TIM2_CH1 | LED 1（第一顆，先暗） | 輸出 (PWM) | 外接 LED + 限流電阻 |
| PA1 | TIM2_CH2 | LED 2（第二顆） | 輸出 (PWM) | 外接 LED + 限流電阻 |
| PB10 | TIM2_CH3 | LED 3（第三顆，最後暗） | 輸出 (PWM) | 外接 LED + 限流電阻 |
| PA2 | LPUART1_TX [STLINKV3E_VCP_RX] | 序列傳送 | 輸出 | 接 ST-LINK VCP |
| PA3 | LPUART1_RX [STLINKV3E_VCP_TX] | 序列接收 | 輸入 | 接 ST-LINK VCP，RX 中斷 |
| PC9 | GPXTI9 | 暫停按鈕 | 輸入 | 下降緣 + 內部上拉，外接按鈕 |
| SWDIO / SWCLK | PA13 / PA14 | 燒錄與除錯 | — | ST-LINK 內建 |

> 注意：NUCLEO-G474RE 板載使用者 LED（LD2）在 PA5、使用者按鈕（B1）在 PC13，
> 本專案**未使用**板載 LD2 / B1，三顆 LED 與暫停鈕皆為**外接**。

---

## 6. 建置與執行：開發環境、燒錄步驟、終端機設定、命令

### 開發環境
- 開發板：**NUCLEO-G474RE**（MCU = STM32G474RET6）
- 工具：**STM32CubeIDE**（專案由 STM32CubeMX 6.18.1 生成）
- 燒錄／供電：板載 ST-LINK（USB 連電腦即可）
- 序列終端機：Tera Term / PuTTY / CubeIDE 內建 Serial，選 ST-LINK VCP 的 COM 埠

### 燒錄步驟
1. 用 STM32CubeIDE 開啟本倉庫的專案資料夾 `weeeeeeeeeeee/we1`。
2. 接上 NUCLEO-G474RE（USB 連 ST-LINK）。
3. 在 CubeIDE 點 **Run → Run**（或 Debug），會自動編譯並經 ST-LINK 燒錄。
4. （倉庫已附 `Debug/` 建置輸出，亦可直接燒錄該 `.elf` / `.bin`。）

### 終端機設定
- 埠：ST-LINK Virtual COM Port（裝置管理員查看）
- 鮑率：**115200**，資料位 8、停止位 1、無奇偶校驗、無硬體流控
- 換行：指令以 `\r` 或 `\n` 結尾分句

### 命令
```
SET <分鐘>    例如：SET 1   設定倒數 1 分鐘，板子回「收到指令: [SET 1]」
START                      開始計時（需先 SET 且分鐘 > 0）
```
結束時板子主動送出 `TIME_UP\r\n`。

---

## 7. 程式結構：哪個檔做什麼、ISR 與主迴圈分工

| 檔案 | 負責什麼 |
| --- | --- |
| `Core/Src/main.c` | 主程式：系統時脈、周邊初始化、主迴圈狀態機、PWM 漸暗邏輯、UART/EXTI 回呼 |
| `Core/Src/stm32g4xx_it.c` | 中斷向量入口：`EXTI9_5_IRQHandler`、`TIM2_IRQHandler`、`LPUART1_IRQHandler` 等 |
| `Core/Src/stm32g4xx_hal_msp.c` | HAL MSP：周邊時脈與腳位底層初始化（`HAL_TIM_MspPostInit` 等） |
| `Core/Src/system_stm32g4xx.c` | 啟動與向量表、系統時脈常數 |
| `Core/Inc/main.h` | 腳位巨集（`LPUART1_TX_Pin` 等）、`HAL_TIM_MspPostInit` 宣告 |
| `Core/Startup/startup_stm32g474retx.s` | 啟動組語、堆疊/堆積與 Reset 向量 |
| `we1.ioc` | STM32CubeMX 設定（腳位、TIM2、LPUART1、時脈 170MHz） |

### ISR 與主迴圈分工
- **主迴圈 `while(1)`**：
  - state ≠ 2 時把三通道 compare 設 0（全暗）並 `HAL_Delay(100)` 空轉。
  - state = 2 時執行漸暗：三燈瞬亮 → PA0→PA1→PB10 各 200 步（每次 −5），
    每步以 `HAL_Delay(5)` 累積延遲；每步與每小步都檢查 `pause_flag`。
  - 三燈全暗後送 `TIME_UP`，`state` 歸 0、`countdown_time` 歸 0。
- **`HAL_UART_RxCpltCallback`（LPUART1）**：逐 byte 收集，遇 `\r`/`\n` 分句；
  解析 `SET %lu`（設 `countdown_time`，`state=1`）與 `START`（`state=2`）；
  回 echo `收到指令: [...]`；收完一定重新呼叫 `HAL_UART_Receive_IT`。
- **`HAL_GPIO_EXTI_Callback`（PC9）**：200ms 軟體消抖後切換 `pause_flag`（暫停/繼續）。


---

## 8. 實作照片：每張附一行圖說

> 本倉庫尚未包含實作照片。請拍攝下列畫面並 **commit 到 `docs/`**，

| 圖片 | 圖說 |
| --- | --- |
| `<img width="500" height="888" alt="image" src="https://github.com/user-attachments/assets/945ff9d4-afb0-4545-a574-60ce37e190f5" /> | NUCLEO-G474RE 接上三顆外接 LED 與 PC9 暫停按鈕的實體接線 |
| `<img width="500" height="888" alt="image" src="https://github.com/user-attachments/assets/f72bd361-97c0-4e63-8f22-96a165093ea8" /> | PA0/PA1/PB10 與 PC9 接線特寫，對得上腳位表 |
| `<img width="500" height="888" alt="image" src="https://github.com/user-attachments/assets/d271aa1e-a35e-4857-abbd-a39821289f6d" />| 三燈依序漸暗進行中的樣子 |
| `<img width="1324" height="913" alt="螢幕擷取畫面 2026-11-03 143642" src="https://github.com/user-attachments/assets/c2024a7d-1004-49e6-8e9c-bcdedcce67cd" /> | 終端機送 `SET 1`、`START` 與板子回 `收到指令`、`TIME_UP` 的畫面 |

---

## 9. AI 使用說明：哪段請 AI、你怎麼查核

| 項目 | 說明 |
| --- | --- |
| 哪些段落讓 AI 做 | README 與期末投影片的整理；TIM2 參數（PSC=169、ARR=999）、腳位表由 AI 對照手冊重算與彙整 |
| 你怎麼查核 | 直接對照 `we1.ioc`（PA0/PA1/PB10=TIM2 CH1/2/3、PA2/PA3=LPUART1、PC9=GPXTI9）與 `Core/Src/main.c`（狀態機、回呼、漸暗迴圈）；並親自演一條驗收（`SET`→`START`→`TIME_UP`→`PC9`） |
| 未讓 AI 決定的部分 | 實體接線、腳位對應、燒錄指令以實際硬體與 CubeIDE 為準；程式邏輯（狀態機、UART 解析、PWM 漸暗）由本人撰寫並查核，AI 僅協助整理與重算參數 |

---

## 10. 已知限制：哪些用替身、換真實要改哪裡

1. **暫停用忙等（busy-wait）**：`while(pause_flag){...}` 會卡住主迴圈，暫停期間無法做其他工作；若要併行，應改用 RTOS 或計時器中斷。
2. **指令為單向**：只有「設定 / 開始」，沒有查詢剩餘時間、取消、暫停的指令；想加取消可新增 `CANCEL` 解析並 `state=0`。
3. **無 RTC / 斷電不保持**：重開機設定消失；換真實可接 32.768kHz LSE（ioc 已預留 RTC 時脈）。
4. **最小單位為「分鐘」**：`SET` 以分鐘計，最小 1 分鐘；要秒級需改指令解析與 `×60000`。
5. **時間用 `×0.88` 硬校正**：`calibrated_ms = total_ms × 88 / 100` 是針對 `HAL_Delay` 累積誤差的經驗值，非精準計時；精準需求應改用硬體定時器中斷計時。
6. **RX 緩衝僅 30 bytes**，且字元過濾只收 32–126；過長指令或夾雜控制字元可能被截斷。
7. **型別混用**：`sscanf(rx_buffer, "SET %lu", &temp_val)` 搭配 `uint32_t`，屬可接受的寫法但需注意平台寬度。
8. **替身未做外殼 / 真實顯示**：LED 與按鈕皆外接，未在板載 LD2 / B1；換真實裝置時，把 LED 替身換成段顯 / 螢幕，把 `TIME_UP` 換成蜂鳴器即可。

---

### 專案資訊
- 專題名稱：怠惰終結器（抗拖延視覺倒數計時器）
- 開發板：NUCLEO-G474RE（STM32G474RET6）
- 工具鏈：STM32CubeMX 6.18.1 / STM32CubeIDE
- 系統時脈：HSI 16MHz → PLL（PLLM÷4、PLLN=85、PLLR÷2）→ SYSCLK 170MHz


