import curses
import pickle
import os
import subprocess

# 字符宽高
CHAR_WIDTH = 5
CHAR_HEIGHT = 7
ASCII_START = 32
ASCII_END = 126

# 字符间隔
H_SPACING = 4  # 水平间隔
V_SPACING = 2  # 垂直间隔

font_data = {
    chr(i): [[0 for _ in range(CHAR_WIDTH)] for _ in range(CHAR_HEIGHT)]
    for i in range(ASCII_START, ASCII_END + 1)
}

def kdialog_input(prompt, title="Input"):
    try:
        result = subprocess.run(['kdialog', '--inputbox', prompt, '--title', title], capture_output=True, text=True)
        return result.stdout.strip()
    except Exception as e:
        return None

def kdialog_file_select(title="Select File", save=False):
    try:
        if save:
            result = subprocess.run(['kdialog', '--getsavefilename', '~/', '--title', title], capture_output=True, text=True)
        else:
            result = subprocess.run(['kdialog', '--getopenfilename', '~/', '--title', title], capture_output=True, text=True)
        return result.stdout.strip()
    except Exception as e:
        return None

def toggle_pixel(char, y, x):
    font_data[char][y][x] = 1 - font_data[char][y][x]

def draw_character(stdscr, char, start_y, start_x, cursor_y=-1, cursor_x=-1, highlight=False):
    max_y, max_x = stdscr.getmaxyx()

    if start_y - 1 >= 0 and start_y + CHAR_HEIGHT < max_y and start_x + CHAR_WIDTH * 2 <= max_x:
        stdscr.addstr(start_y - 1, start_x, f"{char} ({ord(char)})", curses.A_BOLD if highlight else curses.A_NORMAL)

        for y in range(CHAR_HEIGHT):
            for x in range(CHAR_WIDTH):
                if highlight:
                    pixel_char = '██' if font_data[char][y][x] else '░░'
                    attr = curses.A_REVERSE if (y == cursor_y and x == cursor_x) else curses.A_NORMAL
                else:
                    pixel_char = '▓▓' if font_data[char][y][x] else '░░'
                    attr = curses.A_DIM

                stdscr.addstr(start_y + y, start_x + x * 2, pixel_char, attr)


def show_help():
    help_text = (
        "Adafruit GFX Font Editor - Help\n\n"
        "Shortcuts:\n"
        "  - Ctrl + Arrows: Switch between characters\n"
        "  - Arrows: Move the cursor within a character\n"
        "  - SPACE: Toggle pixel on/off\n"
        "  - M: Open settings\n"
        "  - S: Save font data\n"
        "  - L: Load font data\n"
        "  - E: Export font as Adafruit GFX C header\n"
        "  - PgUp/PgDn: Switch pages\n"
        "  - q: Quit\n"
        "\n"
        "Press OK to close this help window."
    )
    try:
        subprocess.run(['kdialog', '--msgbox', help_text, '--title', "Help - Adafruit GFX Font Editor"])
    except Exception as e:
        print(f"Error displaying help: {e}")

def main_menu(stdscr):
    global CHARS_PER_ROW, CHARS_PER_PAGE

    k = 0
    current_page = 0
    current_char_index = 0
    cursor_x, cursor_y = 0, 0

    prev_term_height, prev_term_width = stdscr.getmaxyx()

    def recalculate_layout():
        global CHARS_PER_ROW, CHARS_PER_COL, CHARS_PER_PAGE
        term_height, term_width = stdscr.getmaxyx()
        CHARS_PER_ROW = (term_width + H_SPACING) // (CHAR_WIDTH * 2 + H_SPACING)
        CHARS_PER_COL = (term_height + V_SPACING) // (CHAR_HEIGHT + V_SPACING)
        CHARS_PER_PAGE = CHARS_PER_ROW * CHARS_PER_COL

    recalculate_layout()

    while k != ord('q'):
        term_height, term_width = stdscr.getmaxyx()
        if term_height != prev_term_height or term_width != prev_term_width:
            prev_term_height, prev_term_width = term_height, term_width
            recalculate_layout()

        stdscr.clear()
        stdscr.addstr(0, 0, "Adafruit GFX Font Editor | 'h' to help")

        page_start = current_page * CHARS_PER_PAGE
        page_end = min(page_start + CHARS_PER_PAGE, ASCII_END - ASCII_START + 1)

        for idx in range(page_start, page_end):
            char = chr(ASCII_START + idx)
            row = (idx - page_start) // CHARS_PER_ROW
            col = (idx - page_start) % CHARS_PER_ROW
            start_y = 2 + row * (CHAR_HEIGHT + V_SPACING)
            start_x = col * (CHAR_WIDTH * 2 + H_SPACING)
            highlight = (idx == current_char_index + page_start)
            draw_character(stdscr, char, start_y, start_x, cursor_y if highlight else -1, cursor_x if highlight else -1, highlight=highlight)

        stdscr.refresh()
        k = stdscr.getch()

        if k == 575:  # Ctrl + Up
            if current_char_index >= CHARS_PER_ROW:
                current_char_index -= CHARS_PER_ROW
            elif current_page > 0:
                current_page -= 1
                current_char_index += (CHARS_PER_PAGE - CHARS_PER_ROW)
        elif k == 534:  # Ctrl + Down
            if current_char_index + CHARS_PER_ROW < CHARS_PER_PAGE:
                current_char_index += CHARS_PER_ROW
            elif current_page * CHARS_PER_PAGE + current_char_index + CHARS_PER_ROW < ASCII_END - ASCII_START + 1:
                current_page += 1
                current_char_index -= (CHARS_PER_PAGE - CHARS_PER_ROW)
        elif k == 554:  # Ctrl + Left
            if current_char_index % CHARS_PER_ROW > 0:
                current_char_index -= 1
            elif current_page > 0:
                current_page -= 1
                current_char_index = CHARS_PER_PAGE - 1
        elif k == 569:  # Ctrl + Right
            if (current_char_index + 1) % CHARS_PER_ROW != 0:
                current_char_index += 1
            elif current_page * CHARS_PER_PAGE + current_char_index + 1 < ASCII_END - ASCII_START + 1:
                current_page += 1
                current_char_index = 0

        elif k == curses.KEY_UP and cursor_y > 0:
            cursor_y -= 1
        elif k == curses.KEY_DOWN and cursor_y < CHAR_HEIGHT - 1:
            cursor_y += 1
        elif k == curses.KEY_LEFT and cursor_x > 0:
            cursor_x -= 1
        elif k == curses.KEY_RIGHT and cursor_x < CHAR_WIDTH - 1:
            cursor_x += 1
        elif k == ord(' '):
            char = chr(ASCII_START + page_start + current_char_index)
            toggle_pixel(char, cursor_y, cursor_x)

        elif k == ord('m') or k == ord('M'):
            settings_menu(stdscr)
        elif k == ord('s') or k == ord('S'):
            save_font()
        elif k == ord('l') or k == ord('L'):
            load_font()
        elif k == ord('e') or k == ord('E'):
            export_font_as_c_header(stdscr)
        elif k == ord('h') or k == ord('H'):
            show_help()

        elif k == curses.KEY_NPAGE:  # PgDn
            current_page += 1
            if current_page * CHARS_PER_PAGE >= ASCII_END - ASCII_START + 1:
                current_page = 0
            current_char_index = 0
        elif k == curses.KEY_PPAGE:  # PgUp
            current_page -= 1
            if current_page < 0:
                current_page = (ASCII_END - ASCII_START) // CHARS_PER_PAGE
            current_char_index = 0

def settings_menu(stdscr):
    global CHAR_WIDTH, CHAR_HEIGHT, ASCII_START, ASCII_END, font_data

    settings = [
        ("Character Width", CHAR_WIDTH),
        ("Character Height", CHAR_HEIGHT),
        ("ASCII Start", ASCII_START),
        ("ASCII End", ASCII_END),
    ]
    selected_idx = 0

    while True:
        stdscr.clear()
        stdscr.addstr(0, 0, "Settings Menu (Press Enter to modify, 'q' to quit):", curses.A_BOLD)

        for idx, (setting_name, current_value) in enumerate(settings):
            if idx == selected_idx:
                stdscr.addstr(idx + 2, 2, f"{setting_name}: {current_value}", curses.A_REVERSE)
            else:
                stdscr.addstr(idx + 2, 2, f"{setting_name}: {current_value}")

        stdscr.refresh()

        key = stdscr.getch()
        if key == ord('q'):
            break
        elif key == curses.KEY_UP:
            selected_idx = (selected_idx - 1) % len(settings)
        elif key == curses.KEY_DOWN:
            selected_idx = (selected_idx + 1) % len(settings)
        elif key == ord('\n'):
            setting_name, current_value = settings[selected_idx]
            prompt = f"Enter new value for {setting_name} (Current: {current_value}):"
            input_value = kdialog_input(prompt, title=setting_name)

            if input_value and input_value.isdigit():
                value = int(input_value)
                if setting_name == "Character Width":
                    CHAR_WIDTH = value
                elif setting_name == "Character Height":
                    CHAR_HEIGHT = value
                elif setting_name == "ASCII Start":
                    ASCII_START = max(32, min(value, ASCII_END))
                elif setting_name == "ASCII End":
                    ASCII_END = max(ASCII_START, min(value, 126))

                settings[selected_idx] = (setting_name, value)

                font_data = {
                    chr(i): [[0 for _ in range(CHAR_WIDTH)] for _ in range(CHAR_HEIGHT)]
                    for i in range(ASCII_START, ASCII_END + 1)
                }

def save_font():
    file_path = kdialog_file_select("Select Save Location", save=True)
    if file_path:
        try:
            with open(file_path, 'wb') as f:
                pickle.dump(font_data, f)
            print(f"Font data saved successfully to {file_path}!")
        except Exception as e:
            print(f"Error saving font data: {e}")

def load_font():
    file_path = kdialog_file_select("Select Font File to Load")
    if file_path:
        try:
            with open(file_path, 'rb') as f:
                global font_data, CHAR_WIDTH, CHAR_HEIGHT
                loaded_data = pickle.load(f)

                first_char = next(iter(loaded_data.keys()))
                CHAR_HEIGHT = len(loaded_data[first_char])
                CHAR_WIDTH = len(loaded_data[first_char][0])

                font_data = loaded_data
            print(f"Font data loaded successfully from {file_path} with width {CHAR_WIDTH} and height {CHAR_HEIGHT}!")
        except Exception as e:
            print(f"Error loading font data: {e}")


def export_font_as_c_header(stdscr):
    file_path = kdialog_file_select("Select Export Location", save=True)
    if not file_path:
        return

    font_name = kdialog_input("Enter font name:", "Font Name")
    if not font_name:
        font_name = "customFont"

    try:
        glyph_data = []
        bitmap_data = []
        bitmap_offset = 0
        lines = []

        for char_code in range(ASCII_START, ASCII_END + 1):
            char = chr(char_code)
            char_bitmap = font_data[char]

            width = CHAR_WIDTH
            height = CHAR_HEIGHT
            x_advance = CHAR_WIDTH + 1
            x_offset = 0
            y_offset = -CHAR_HEIGHT
            y_advance = CHAR_HEIGHT + 1

            lines.append(f"  // '{char}', {width}x{height}")

            bit_buffer = 0
            bit_count = 0
            glyph_bitmap = []

            for row in char_bitmap:
                for bit in row:
                    bit_buffer = (bit_buffer << 1) | bit
                    bit_count += 1

                    if bit_count == 8:
                        glyph_bitmap.append(bit_buffer)
                        bit_buffer = 0
                        bit_count = 0

            if bit_count > 0:
                bit_buffer <<= (8 - bit_count)
                glyph_bitmap.append(bit_buffer)

            hex_string = ', '.join(f"0x{byte:02X}" for byte in glyph_bitmap)
            lines.append(f"  {hex_string},")

            bitmap_data.extend(glyph_bitmap)

            glyph_data.append({
                "bitmapOffset": bitmap_offset,
                "width": width,
                "height": height,
                "xAdvance": x_advance,
                "xOffset": x_offset,
                "yOffset": y_offset
            })
            bitmap_offset += len(glyph_bitmap)

        with open(file_path, "w") as f:
            f.write(f"#ifndef {font_name.upper()}_H_\n#define {font_name.upper()}_H_\n\n")
            f.write("#include <Adafruit_GFX.h>\n\n")
            f.write("// Automatically generated font header for Adafruit GFX\n\n")
            f.write(f"const uint8_t {font_name}Bitmaps[] = {{\n")
            f.write('\n'.join(lines))
            f.write("\n};\n\n")

            f.write(f"const GFXglyph {font_name}Glyphs[] = {{\n")
            for glyph in glyph_data:
                f.write(f"  {{ {glyph['bitmapOffset']}, {glyph['width']}, {glyph['height']}, "
                        f"{glyph['xAdvance']}, {glyph['xOffset']}, {glyph['yOffset']} }},\n")
            f.write("};\n\n")

            f.write(f"const GFXfont {font_name} = {{\n"
                    f"  (uint8_t *){font_name}Bitmaps,\n"
                    f"  (GFXglyph *){font_name}Glyphs,\n"
                    f"  {ASCII_START}, {ASCII_END}, {y_advance}\n"
                    f"}};\n\n")

            f.write(f"#endif // {font_name.upper()}_H_\n")

        print(f"Font successfully exported to {file_path} as {font_name}")
    except Exception as e:
        print(f"Error exporting font: {e}")


curses.wrapper(main_menu)
