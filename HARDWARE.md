# Màn hình 2.8 inch ESP32-S3 Display

Tài liệu tổng hợp từ [LCD Wiki – 2.8inch ESP32-S3 Display](https://www.lcdwiki.com/2.8inch_ESP32-S3_Display) (cập nhật wiki: 16/03/2026). Chi tiết đầy đủ, sơ đồ và file tải về nên xem trực tiếp trên trang wiki.

## Tổng quan

Module **ESP32-S3** tích hợp màn **IPS TFT 2.8"**, độ phân giải **240×320**, giao tiếp **4-line SPI** (driver **ILI9341V**). Có phiên bản **cảm ứng điện dung** (SKU **ES3C28P**) và phiên bản **không cảm ứng** (**ES3N28P**).

**Slideshow:** TFT_eSPI dùng **`ILI9341_DRIVER`**; sau `setRotation` gọi **`ili9341v_apply_lcd_wiki_post_init()`** (`ili9341v_tune.cpp`) để ghi lại nguồn / VCOM (**`0xC5`/`0xC7`** với **`0xBD`** cho VCM2), **`0x21`**, **`0xF6`/`0xB1`**, gamma **`0xE0`/`0xE1`** theo [ILI9341V_Init.txt](https://www.lcdwiki.com/res/ES3C28P/ILI9341V_Init.txt) — **không ghi `0xB6`** để giữ thứ tự quét của TFT_eSPI (wiki dùng `0xB6` khác, dễ gây lật dọc).

## Đặc điểm nổi bật

- MCU ESP32-S3, tài nguyên phát triển phong phú
- Màu tối đa **262K (RGB666)**, thường dùng **65K (RGB565)**
- Nhiều cổng mở rộng (I2C, UART, GPIO…)
- Loa ngoài, micro thu âm
- LED RGB báo trạng thái
- Cảm ứng điện dung (bản ES3C28P)
- **USB Type-C**: nạp chương trình và cấp nguồn (mạch tải một phím, có thể vào download mode không cần giữ BOOT)
- Khe **Micro SD / TF**
- Pin Li-ion **3.7V** ngoài + mạch sạc/xả an toàn
- Ví dụ mẫu Arduino / MicroPython / ESP-IDF
- Hỗ trợ ứng dụng giọng nói AI **“Xiaozhi”** (theo mô tả wiki)

## Thông số ESP32-S3 (N16R8)

| Mục | Thông số |
| --- | --- |
| Module | ESP32-S3 |
| CPU | Xtensa LX7 dual-core 32-bit |
| Xung tối đa | 240 MHz |
| Bộ nhớ | 384 KB ROM + 512 KB SRAM + 16 KB RTC SRAM + **8 MB OPI PSRAM** + **16 MB SPI Flash** |
| Wi-Fi | 2.4 GHz, 802.11 b/g/n |
| Bluetooth | 5.0 BR/EDR và BLE |
| Điện áp hoạt động | 3.0–3.6 V |

## Thông số màn hình LCD

| Mục | Thông số |
| --- | --- |
| Kích thước | 2.8 inch |
| Loại | IPS TFT |
| Độ phân giải | 240 × RGB × 320 (pixel) |
| Vùng hiển thị | 43.20 × 57.60 mm (W × H) |
| Driver | ILI9341V |
| Giao tiếp | 4-line SPI |
| Kích pixel | 0.153 × 0.153 mm |
| Góc nhìn | ALL 0’CLOCK |
| Độ sáng (typ) | 280 cd/m² |
| Đèn nền | 4 LED trắng |
| Nhiệt độ vận hành / bảo quản | −30 °C ~ 80 °C |

## Thông số cảm ứng (ES3C28P)

| Mục | Thông số |
| --- | --- |
| Loại | Cảm ứng điện dung |
| Vùng hợp lệ | 240 × 320 pixel |
| Driver | D-FT6336G |
| Giao tiếp | I2C |
| Vùng nhìn thấy | 45.20 × 59.45 mm |

## Kích thước & điện

**Kích module (LCD):** 50.00 ± 0.2 × 69.20 ± 0.2 mm; độ dày ~2.3 mm (không kể đăng ký/keo) — bản có cảm ứng mỏng hơn (~1.2 mm phần touch stack theo wiki).

**Kích sản phẩm hoàn chỉnh:** có touch ~50 × 86 × 10.6 mm; không touch ~50 × 86 × 9.1 mm.

**Sạc pin:** điện áp sạc 4.2–6.5 V (typ 5 V); dòng tối đa 500 mA, module ~290 mA; điện áp bão hòa ~4.24 V; pin khuyến nghị: Li-po 3.7 V.

**Điện năng tiêu thụ (tham khảo wiki):** chỉ hiển thị ~140 mA @ 5 V; hiển thị + loa + sạc pin ~560 mA; công suất khoảng 0.7 W (chỉ LCD) / ~2.8 W (đầy đủ). Dòng đèn nền ~79 mA.

**SKU & khối lượng:** ES3C28P ~111 g; ES3N28P ~100 g.

## Cổng & chức năng trên board

| Thành phần | Mô tả ngắn |
| --- | --- |
| ESP32-S3 | MCU điều khiển toàn bộ |
| MicroSD | Mở rộng lưu trữ (font, ảnh, âm thanh…) |
| RGB | LED 3 màu, điều khiển qua một chân IO |
| UART (1.25 mm 4P) | Debug/serial; cần module USB–TTL ngoài nếu dùng cổng này |
| Pin (1.25 mm 2P) | Pin Li-po 3.7 V; **chú ý cực tính** |
| BOOT | Vào download mode (giữ BOOT khi cấp nguồn rồi thả; hoặc giữ BOOT → nhấn RESET → thả RESET → thả BOOT) |
| Type-C | Nguồn + nạp firmware (hỗ trợ auto download) |
| RESET | Reset ESP32-S3 và LCD (dùng chung mạch reset) |
| Mở rộng (1.25 mm 4P) | GPIO **2, 3, 14, 21** |
| Loa (1.25 mm 2P) | Loa tối đa ~1.5 W (8 Ω) hoặc ~2 W (4 Ω) |
| I2C (1.25 mm 4P) | Bus I2C **dùng chung với touch**; có thể dùng làm GPIO thường nếu không xung đột |

> Wiki gốc có đoạn mô tả I2C/SPI/MicroSD lẫn nhau; thực tế layout và datasheet schematic trên wiki là nguồn chính xác nhất.

## Phân bổ GPIO (chi tiết)

Góc nhìn **ESP32-S3 là master**: dữ liệu **ra khỏi** chip → **MOSI** (Master Out); dữ liệu **vào** chip ← **MISO** (Master In). Wiki gọi IO11 là *write data* (= đường MOSI tới LCD), IO13 là *read data* (= MISO từ LCD khi đọc RAM/pixel — thư viện chỉ ghi thì có thể không dùng MISO).

### LCD — bus SPI 4 dây (ILI9341V)

| GPIO | Tín hiệu wiki | Tên thường gặp | Mô tả |
| --- | --- | --- | --- |
| **IO12** | SPI clock | **SCK / CLK** | Xung nhịp SPI |
| **IO11** | SPI write data | **MOSI** (SDI phía LCD) | Dữ liệu ESP32 → màn hình (lệnh + pixel) |
| **IO13** | SPI read data | **MISO** (SDO phía LCD) | Dữ liệu màn hình → ESP32 khi đọc; nhiều demo chỉ dùng half-duplex ghi |
| **IO10** | Chip select | **CS** | Active **low** — chọn chip LCD |
| **IO46** | Command/Data | **DC** hay **D/C** | **Cao** = dữ liệu (RAM/pixel), **thấp** = lệnh |
| **EN** (nút RST) | LCD reset | **RST** | Reset **active low**, **chung** với reset ESP32-S3 (wiki: share với main control) |
| **IO45** | Backlight | **BL** | **Cao** = bật đèn nền, **thấp** = tắt |

Tóm tắt nhanh SPI LCD: **SCK=IO12, MOSI=IO11, MISO=IO13, CS=IO10, DC=IO46, RST=EN, BL=IO45**.

### Cảm ứng — I2C (FT6336G, bản ES3C28P)

| GPIO | Chức năng | Ghi chú |
| --- | --- | --- |
| **IO16** | **SDA** | Data I2C (touch + header I2C ngoài **cùng bus** — tránh địa chỉ xung đột) |
| **IO15** | **SCL** | Clock I2C |
| **IO18** | Touch RST | Reset touch, **active low** |
| **IO17** | Touch INT | Ngắt khi có sự kiện chạm; wiki: **active low** khi có touch |

### Thẻ nhớ — SDIO (4 bit)

| GPIO | Vai trò SDIO | Ghi chú |
| --- | --- | --- |
| **IO38** | **CLK** | Clock thẻ |
| **IO40** | **CMD** | Lệnh |
| **IO39** | **DATA0** | Dữ liệu bit 0 |
| **IO41** | **DATA1** | Dữ liệu bit 1 |
| **IO48** | **DATA2** | Dữ liệu bit 2 |
| **IO47** | **DATA3** | Dữ liệu bit 3 |

### Âm thanh — I2S + bật loa

| GPIO | Wiki (tiếng Anh) | Tên gợi ý I2S | Mô tả |
| --- | --- | --- | --- |
| **IO1** | Audio output enable | **PA enable / AMP_EN** | **Thấp** = bật khuếch đại loa, **cao** = tắt (theo wiki) |
| **IO4** | I2S master clock | **MCLK** | Clock master codec/I2S |
| **IO5** | I2S bit clock | **BCLK** | Bit clock |
| **IO6** | I2S data out | **DOUT** (ESP → DAC/amp) | Dữ liệu âm thanh phát ra |
| **IO7** | LR / channel select | **WS** hay **LRCK** | **Cao** = kênh phải, **thấp** = kênh trái (theo wiki) |
| **IO8** | I2S data in | **DIN** (mic → ESP) | Thu âm vào |

**Codec (ES8311):** Đường âm thanh phát ra loa đi qua **codec ES8311** (điều khiển qua **I2C**) và khuếch đại (datasheet wiki thường nêu **FM8002E**). Phần sóng đi vào DAC vẫn **cần MCU cấu hình thanh ghi codec qua I2C** — riêng xung I2S không thay được bước đó.

| Mục | Giá trị / ghi chú |
| --- | --- |
| Chip | **ES8311** |
| Địa chỉ I2C (7-bit) | **0x18** (tra schematic/manual nếu có nhảy CE) |
| Bus I2C | **Chung với cảm ứng:** SDA **IO16**, SCL **IO15** — mỗi thiết bị một địa chỉ (touch FT6336 thường **0x38**) |
| Dữ liệu âm thanh | **I2S** qua MCLK/BCLK/WS/DOUT (và DIN khi thu mic), bả GPIO ở trên |
| PA / loa | **IO1** — wiki: **mức thấp = bật** khuếch đại |

### Serial, pin, LED, nút

| GPIO / chân | Chức năng |
| --- | --- |
| **IO43** | **UART0 RX** (RXD0) |
| **IO44** | **UART0 TX** (TXD0) |
| **IO9** | **ADC** đo điện áp pin (BATTERY) |
| **IO42** | **RGB** LED một dây (điều khiển R/G/B theo giao thức LED đơn dây) |
| **IO0** | Nút **BOOT** — download mode (giữ khi cấp nguồn / tổ hợp với RESET như mục cổng ở trên) |
| **EN** | Reset SoC + LCD (mạch chung) |

### GPIO đưa ra header mở rộng (1.25 mm 4P)

| GPIO | Ghi chú |
| --- | --- |
| **IO2** | GPIO tự do (kiểm tra strap/bootloader nếu dùng làm input đặc biệt) |
| **IO3** | GPIO tự do |
| **IO14** | GPIO tự do |
| **IO21** | GPIO tự do |

### Xung đột & lưu ý khi cấu hình phần mềm

- **I2C touch (IO16/15)** trùng với **header I2C** ngoài — mọi thiết bị trên bus phải khác **địa chỉ 7-bit**; touch FT6336 thường **0x38** (cần đối chiếu datasheet/demo).
- **MISO (IO13)** chỉ cần kết nối đúng nếu driver/thư viện có **đọc** từ LCD; cấu hình SPI chỉ ghi có thể bỏ qua đọc nhưng chân vẫn được board dùng cho LCD.
- **IO0** dùng cho BOOT: tránh kéo mức sai lúc reset nếu gắn ngoại vi nặng.
- Bảng Excel chính thức trên wiki: [ESP32-S3 I/O resource allocation table](https://www.lcdwiki.com/res/ES3C28P/ESP32-S3%E8%8A%AF%E7%89%87IO%E8%B5%84%E6%BA%90%E5%88%86%E9%85%8D%E8%A1%A8.xlsx).

## Tài nguyên tải về & tài liệu (liên kết wiki)

- **Gói dữ liệu / demo:** xem mục *Data pack download* trên wiki (Baidu: mã giải nén `yvne`; 123pan và link cloud khác trên trang).
- **Bảng phân bổ IO (Excel):** [ESP32-S3 I/O resource allocation table](https://www.lcdwiki.com/res/ES3C28P/ESP32-S3%E8%8A%AF%E7%89%87IO%E8%B5%84%E6%BA%90%E5%88%86%E9%85%8D%E8%A1%A8.xlsx)
- **Mã khởi tạo ILI9341:** [ILI9341V_Init.txt](https://www.lcdwiki.com/res/ES3C28P/ILI9341V_Init.txt)
- **Môi trường:** Arduino IDE, MicroPython, ESP-IDF, LVGL trên ESP-IDF — PDF hướng dẫn trong mục *References* trên wiki.
- **Datasheet tham khảo:** ESP32-S3 TRM, TP4054 (sạc), FM8002E (amp), v.v. — liệt kê đầy đủ tại wiki.
- **Công cụ:** Flash Download Tool (Espressif / mirror LCD Wiki).

## Hỗ trợ (theo wiki)

Email: Lcdwiki@163.com, goodtft@163.com

---

*Nội dung tổng hợp phục vụ tra cứu nhanh; mọi thông số chính thức lấy theo bản cập nhật mới nhất trên [LCD Wiki](https://www.lcdwiki.com/2.8inch_ESP32-S3_Display).*
