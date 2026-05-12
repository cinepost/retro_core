import tkinter as tk

# Standard NES 64-color palette
NES_PALETTE = [
    "#666666", "#002a88", "#1412a7", "#3b00a4", "#5c007e", "#6e0040", "#6c0600", "#561d00",
    "#333500", "#0b4800", "#005200", "#004f08", "#00404d", "#000000", "#000000", "#000000",
    "#adadad", "#155fd9", "#4240ff", "#7527fe", "#a01acc", "#b71e7b", "#b53120", "#994e00",
    "#6b6d00", "#388700", "#0c9300", "#008f32", "#007c8d", "#000000", "#000000", "#000000",
    "#ffffff", "#64b0ff", "#9290ff", "#c676ff", "#f36aff", "#fe6ecc", "#fe8170", "#ea9e22",
    "#bcbe00", "#88d800", "#5ce430", "#45e082", "#48cdde", "#4f4f4f", "#000000", "#000000",
    "#ffffff", "#c0dfff", "#d3d2ff", "#e8c8ff", "#fbc2ff", "#fbcce5", "#fbc2bf", "#f5d2a4",
    "#dfdf88", "#cae888", "#b9f09d", "#b0f0c0", "#b0ebef", "#b8b8b8", "#000000", "#000000"
]

class NESSpriteEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("NES CHR Editor")
        
        # CHR Bank: 256 sprites, each is an 8x8 array
        self.chr_bank = [[[0 for _ in range(8)] for _ in range(8)] for _ in range(256)]
        self.active_sprite_idx = 0
        
        self.subpalettes = [[0x0F, 0x30, 0x16, 0x01] for _ in range(4)]
        self.active_sub_idx = 0
        self.selected_color_idx = 1
        
        self.setup_ui()

    def setup_ui(self):
        # --- Left Side: CHR Bank Grid ---
        self.bank_frame = tk.Frame(self.root)
        self.bank_frame.grid(row=0, column=0, padx=10, pady=10, sticky="ns")
        tk.Label(self.bank_frame, text="CHR Bank (16x16)").pack()
        
        self.bank_canvas = tk.Canvas(self.bank_frame, width=256, height=256, bg="#000")
        self.bank_canvas.pack()
        self.bank_canvas.bind("<Button-1>", self.select_sprite_from_bank)

        # --- Center: Main Editor ---
        self.editor_frame = tk.Frame(self.root)
        self.editor_frame.grid(row=0, column=1, padx=10, pady=10)
        
        self.canvas = tk.Canvas(self.editor_frame, width=320, height=320, bg="#333")
        self.canvas.pack()
        self.canvas.bind("<B1-Motion>", self.paint)
        self.canvas.bind("<Button-1>", self.paint)

        # --- Right Side: Controls ---
        self.side_panel = tk.Frame(self.root)
        self.side_panel.grid(row=0, column=2, sticky="n", padx=10)

        tk.Label(self.side_panel, text="Subpalette:").pack()
        self.sub_var = tk.IntVar(value=0)
        for i in range(4):
            tk.Radiobutton(self.side_panel, text=f"Set {i}", variable=self.sub_var, 
                           value=i, command=self.refresh_all).pack(anchor="w")

        tk.Label(self.side_panel, text="\nColors (Left=Pick, Right=Draw):").pack()
        self.palette_btns = []
        for i in range(4):
            btn = tk.Button(self.side_panel, width=10, command=lambda idx=i: self.open_nes_picker(idx))
            btn.pack(pady=2)
            btn.bind("<Button-3>", lambda e, idx=i: self.select_draw_color(idx))
            self.palette_btns.append(btn)

        tk.Button(self.root, text="Export Full CHR (4KB)", command=self.export_chr).grid(row=1, column=1, pady=10)
        
        self.refresh_all()

    def select_sprite_from_bank(self, event):
        col, row = event.x // 16, event.y // 16
        self.active_sprite_idx = row * 16 + col
        self.refresh_all()

    def select_draw_color(self, idx):
        self.selected_color_idx = idx

    def paint(self, event):
        x, y = event.x // 40, event.y // 40
        if 0 <= x < 8 and 0 <= y < 8:
            self.chr_bank[self.active_sprite_idx][y][x] = self.selected_color_idx
            self.refresh_all()

    def refresh_all(self):
        self.active_sub_idx = self.sub_var.get()
        self.draw_bank()
        self.draw_editor()
        self.update_palette_buttons()

    def draw_bank(self):
        self.bank_canvas.delete("all")
        sub = self.subpalettes[self.active_sub_idx]
        for s_idx in range(256):
            s_row, s_col = divmod(s_idx, 16)
            for y in range(8):
                for x in range(8):
                    c_idx = self.chr_bank[s_idx][y][x]
                    color = NES_PALETTE[sub[c_idx]] if c_idx != 0 else "#000"
                    # Draw 2x2 pixels for the bank preview
                    px, py = (s_col * 16) + (x * 2), (s_row * 16) + (y * 2)
                    self.bank_canvas.create_rectangle(px, py, px+2, py+2, fill=color, outline="")
        
        # Highlight active sprite
        sr, sc = divmod(self.active_sprite_idx, 16)
        self.bank_canvas.create_rectangle(sc*16, sr*16, sc*16+16, sr*16+16, outline="red", width=2)

    def draw_editor(self):
        self.canvas.delete("all")
        sprite = self.chr_bank[self.active_sprite_idx]
        sub = self.subpalettes[self.active_sub_idx]
        for y in range(8):
            for x in range(8):
                c_idx = sprite[y][x]
                color = NES_PALETTE[sub[c_idx]] if c_idx != 0 else "#222"
                self.canvas.create_rectangle(x*40, y*40, (x+1)*40, (y+1)*40, fill=color, outline="#444")

    def update_palette_buttons(self):
        sub = self.subpalettes[self.active_sub_idx]
        for i, btn in enumerate(self.palette_btns):
            btn.config(bg=NES_PALETTE[sub[i]], text=f"Index {i}")

    def open_nes_picker(self, sub_pos):
        picker = tk.Toplevel(self.root)
        for i, color in enumerate(NES_PALETTE):
            r, c = divmod(i, 8)
            tk.Button(picker, bg=color, width=3, command=lambda v=i: self.set_color(sub_pos, v, picker)).grid(row=r, column=c)

    def set_color(self, pos, val, win):
        self.subpalettes[self.active_sub_idx][pos] = val
        win.destroy()
        self.refresh_all()

    def export_chr(self):
        """Exports 256 sprites into a 4096-byte binary blob."""
        full_bin = bytearray()
        for sprite in self.chr_bank:
            p0, p1 = [], []
            for row in sprite:
                b0, b1 = 0, 0
                for col in row:
                    b0 = (b0 << 1) | (col & 1)
                    b1 = (b1 << 1) | ((col >> 1) & 1)
                p0.append(b0)
                p1.append(b1)
            full_bin.extend(p0 + p1)
        
        with open("output.chr", "wb") as f:
            f.write(full_bin)
        print("Saved 4KB CHR data to output.chr")

if __name__ == "__main__":
    root = tk.Tk()
    app = NESSpriteEditor(root)
    root.mainloop()
