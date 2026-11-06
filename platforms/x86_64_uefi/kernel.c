#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define COM1 0x3F8u
#define PS2_DATA 0x60u
#define PS2_STATUS 0x64u
#define MB2_BOOT_MAGIC 0x36D76289u
#define MB2_TAG_FRAMEBUFFER 8u
#define HISTORY_LINES 64u
#define LINE_CAPACITY 128u
#define UI_MARGIN 16u
#define UI_TOP 48u
#define CELL_W 6u
#define CELL_H 9u

struct mb2_info { uint32_t total_size, reserved; };
struct mb2_tag { uint32_t type, size; };
struct mb2_framebuffer_tag {
    uint32_t type, size;
    uint64_t address;
    uint32_t pitch, width, height;
    uint8_t bpp, framebuffer_type;
    uint16_t reserved;
    union {
        struct { uint8_t red_pos, red_size, green_pos, green_size, blue_pos, blue_size; } rgb;
        struct { uint32_t palette_addr; uint16_t palette_num; } indexed;
    } color;
};
struct screen {
    volatile uint8_t *pixels;
    uint32_t width, height, pitch;
    uint8_t bpp, red_pos, red_size, green_pos, green_size, blue_pos, blue_size;
};

static struct screen g_screen;
static char g_lines[HISTORY_LINES][LINE_CAPACITY];
static uint32_t g_line_count, g_line_head, g_cursor, g_columns;
static bool g_caps_lock, g_shift;
static bool g_mouse_ready, g_mouse_packet_started;
static uint8_t g_mouse_packet[3], g_mouse_packet_index, g_mouse_buttons;
static int32_t g_mouse_x, g_mouse_y;

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void serial_init(void)
{
    outb(COM1 + 1, 0); outb(COM1 + 3, 0x80); outb(COM1, 1);
    outb(COM1 + 1, 0); outb(COM1 + 3, 3); outb(COM1 + 2, 0xC7); outb(COM1 + 4, 0x0B);
}
static void serial_putc(char ch)
{
    for (uint32_t n=0;n<100000u;++n) if (inb(COM1+5)&0x20u) { outb(COM1,(uint8_t)ch); return; }
}
static void serial_write(const char *s)
{
    while (*s) { if (*s=='\n') serial_putc('\r'); serial_putc(*s++); }
}

static uint32_t rgb(uint8_t r,uint8_t g,uint8_t b)
{
    uint32_t v=0;
    if(g_screen.red_size && g_screen.red_size<=8) v|=((uint32_t)(r>>(8-g_screen.red_size)))<<g_screen.red_pos;
    if(g_screen.green_size && g_screen.green_size<=8) v|=((uint32_t)(g>>(8-g_screen.green_size)))<<g_screen.green_pos;
    if(g_screen.blue_size && g_screen.blue_size<=8) v|=((uint32_t)(b>>(8-g_screen.blue_size)))<<g_screen.blue_pos;
    return v;
}
static void pixel(uint32_t x,uint32_t y,uint32_t color)
{
    if(!g_screen.pixels || x>=g_screen.width || y>=g_screen.height) return;
    volatile uint8_t *p=g_screen.pixels+(size_t)y*g_screen.pitch+(size_t)x*(g_screen.bpp/8u);
    p[0]=(uint8_t)color; p[1]=(uint8_t)(color>>8); p[2]=(uint8_t)(color>>16);
    if(g_screen.bpp==32) p[3]=(uint8_t)(color>>24);
}
static void rect(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t color)
{
    if(x>=g_screen.width || y>=g_screen.height) return;
    if(w>g_screen.width-x) w=g_screen.width-x;
    if(h>g_screen.height-y) h=g_screen.height-y;
    for(uint32_t yy=y;yy<y+h;++yy) for(uint32_t xx=x;xx<x+w;++xx) pixel(xx,yy,color);
}

/* 5x7 column font for the shell's letters and digits. */
static void glyph(char ch,uint8_t c[5])
{
    static const uint8_t font[36][5]={
        {0x3e,0x51,0x49,0x45,0x3e},{0x00,0x42,0x7f,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4b,0x31},{0x18,0x14,0x12,0x7f,0x10},
        {0x27,0x45,0x45,0x45,0x39},{0x3c,0x4a,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},{0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1e},
        {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},{0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},
        {0x7f,0x09,0x09,0x09,0x01},{0x3e,0x41,0x49,0x49,0x7a},{0x7f,0x08,0x08,0x08,0x7f},{0x00,0x41,0x7f,0x41,0x00},{0x20,0x40,0x41,0x3f,0x01},
        {0x7f,0x08,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},{0x7f,0x02,0x0c,0x02,0x7f},{0x7f,0x04,0x08,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},
        {0x7f,0x09,0x09,0x09,0x06},{0x3e,0x41,0x51,0x21,0x5e},{0x7f,0x09,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7f,0x01,0x01},
        {0x3f,0x40,0x40,0x40,0x3f},{0x1f,0x20,0x40,0x20,0x1f},{0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,0x08,0x14,0x63},{0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43}
    };
    if(ch>='a'&&ch<='z') ch=(char)(ch-'a'+'A');
    if(ch>='0'&&ch<='9') { for(int i=0;i<5;++i)c[i]=font[ch-'0'][i]; return; }
    if(ch>='A'&&ch<='Z') { for(int i=0;i<5;++i)c[i]=font[10+ch-'A'][i]; return; }
    for(int i=0;i<5;++i)c[i]=0;
    switch(ch) {
    case ':': c[1]=c[2]=0x36; break; case '.': c[1]=c[2]=0x60; break;
    case ',': c[1]=0x40;c[2]=0x20;break; case '-': for(int i=0;i<5;++i)c[i]=0x08;break;
    case '/': c[0]=0x20;c[1]=0x10;c[2]=8;c[3]=4;c[4]=2;break;
    case '>': c[1]=0x41;c[2]=0x22;c[3]=0x14;c[4]=8;break;
    case '?': c[0]=2;c[1]=1;c[2]=0x51;c[3]=9;c[4]=6;break;
    case '>': break; case ' ': break;
    case '>': break;
    default: break;
    }
}
static void draw_char(uint32_t x,uint32_t y,char ch,uint32_t color)
{
    uint8_t c[5]; glyph(ch,c);
    for(uint32_t xx=0;xx<5;++xx) for(uint32_t yy=0;yy<7;++yy) if(c[xx]&(1u<<yy)) pixel(x+xx,y+yy,color);
}
static void draw_text(uint32_t x,uint32_t y,const char *s,uint32_t color)
{
    while(*s){draw_char(x,y,*s++,color);x+=CELL_W;}
}
static void draw_mouse(void)
{
    if(!g_screen.pixels || !g_mouse_ready) return;
    uint32_t edge=rgb(5,8,12), fill=(g_mouse_buttons&1u)?rgb(255,155,60):rgb(255,255,255);
    for(int32_t dy=0;dy<12;++dy) for(int32_t dx=0;dx<9;++dx) {
        if(dx>dy/2+1) continue;
        uint32_t color=(dx==0 || dx==dy/2+1 || dy==11)?edge:fill;
        int32_t px=g_mouse_x+dx, py=g_mouse_y+dy;
        if(px>=0&&py>=0) pixel((uint32_t)px,(uint32_t)py,color);
    }
}
static uint32_t visible_rows(void)
{
    uint32_t rows=g_screen.height>UI_TOP+16?(g_screen.height-UI_TOP-16)/CELL_H:1;
    return rows>HISTORY_LINES?HISTORY_LINES:rows;
}
static void screen_redraw(void)
{
    if(!g_screen.pixels) return;
    uint32_t bg=rgb(15,22,34),panel=rgb(23,33,49),header=rgb(30,111,190),fg=rgb(230,239,250),accent=rgb(94,205,255);
    rect(0,0,g_screen.width,g_screen.height,bg); rect(0,0,g_screen.width,36,header);
    draw_text(16,13,"DEVOS X86-64 PC KERNEL",fg);
    rect(8,44,g_screen.width>16?g_screen.width-16:g_screen.width,g_screen.height>52?g_screen.height-52:g_screen.height,panel);
    uint32_t rows=visible_rows(),show=g_line_count<rows?g_line_count:rows;
    uint32_t start=(g_line_head+HISTORY_LINES-show)%HISTORY_LINES;
    for(uint32_t i=0;i<show;++i) draw_text(UI_MARGIN,UI_TOP+i*CELL_H,g_lines[(start+i)%HISTORY_LINES],fg);
    draw_mouse();
}
static void new_line(void)
{
    g_line_head=(g_line_head+1u)%HISTORY_LINES;
    if(g_line_count<HISTORY_LINES)++g_line_count;
    g_lines[g_line_head][0]='\0';g_cursor=0;
}
static void console_putc(char ch)
{
    if(ch=='\r')return;
    if(ch=='\b') { if(g_cursor){g_lines[g_line_head][--g_cursor]='\0';} screen_redraw(); return; }
    if(ch=='\n') { serial_write("\n");new_line();screen_redraw();return; }
    if(g_cursor>=LINE_CAPACITY-1||g_cursor>=g_columns)new_line();
    if(g_cursor<LINE_CAPACITY-1){g_lines[g_line_head][g_cursor++]=ch;g_lines[g_line_head][g_cursor]='\0';}
    serial_putc(ch);screen_redraw();
}
static void console_write(const char *s){while(*s)console_putc(*s++);}

static bool framebuffer_init(uint32_t address)
{
    if(!address||address>0xfffff000u)return false;
    const struct mb2_info *info=(const struct mb2_info *)(uintptr_t)address;
    if(info->total_size<16||info->total_size>0x01000000u)return false;
    for(uint32_t off=8;off+sizeof(struct mb2_tag)<=info->total_size;) {
        const struct mb2_tag *tag=(const struct mb2_tag *)(uintptr_t)(address+off);
        if(tag->size<8||off+tag->size>info->total_size)return false;
        if(tag->type==MB2_TAG_FRAMEBUFFER&&tag->size>=sizeof(struct mb2_framebuffer_tag)) {
            const struct mb2_framebuffer_tag *fb=(const struct mb2_framebuffer_tag *)tag;
            if(fb->framebuffer_type!=1||(fb->bpp!=24&&fb->bpp!=32)||fb->address>=0x100000000ull)return false;
            uint32_t bytes=fb->bpp/8u;
            if(fb->width<320||fb->height<200||fb->pitch<(uint64_t)fb->width*bytes||fb->address+(uint64_t)fb->pitch*fb->height>0x100000000ull)return false;
            g_screen.pixels=(volatile uint8_t *)(uintptr_t)fb->address;g_screen.width=fb->width;g_screen.height=fb->height;g_screen.pitch=fb->pitch;g_screen.bpp=fb->bpp;
            g_screen.red_pos=fb->color.rgb.red_pos;g_screen.red_size=fb->color.rgb.red_size;g_screen.green_pos=fb->color.rgb.green_pos;g_screen.green_size=fb->color.rgb.green_size;g_screen.blue_pos=fb->color.rgb.blue_pos;g_screen.blue_size=fb->color.rgb.blue_size;
            if(g_screen.red_pos+g_screen.red_size>fb->bpp||g_screen.green_pos+g_screen.green_size>fb->bpp||g_screen.blue_pos+g_screen.blue_size>fb->bpp)return false;
            g_columns=(g_screen.width-UI_MARGIN*2)/CELL_W;if(g_columns>=LINE_CAPACITY)g_columns=LINE_CAPACITY-1;if(!g_columns)g_columns=1;
            g_mouse_x=(int32_t)(g_screen.width/2);g_mouse_y=(int32_t)(g_screen.height/2);return true;
        }
        if(tag->type==0)break;
        off+=(tag->size+7u)&~7u;
    }
    return false;
}

static bool ps2_wait_write(void)
{
    for(uint32_t n=0;n<100000u;++n)if((inb(PS2_STATUS)&2u)==0)return true;
    return false;
}
static bool ps2_wait_read(uint8_t *value)
{
    for(uint32_t n=0;n<100000u;++n)if(inb(PS2_STATUS)&1u){*value=inb(PS2_DATA);return true;}
    return false;
}
static bool ps2_command(uint8_t command)
{
    if(!ps2_wait_write())return false;outb(PS2_STATUS,command);return true;
}
static bool ps2_data(uint8_t value)
{
    if(!ps2_wait_write())return false;outb(PS2_DATA,value);return true;
}
static bool mouse_command(uint8_t command)
{
    uint8_t ack;
    return ps2_command(0xD4)&&ps2_data(command)&&ps2_wait_read(&ack)&&ack==0xFA;
}
static bool mouse_init(void)
{
    uint8_t command_byte;
    if(!ps2_command(0xA8)||!ps2_command(0x20)||!ps2_wait_read(&command_byte))return false;
    command_byte=(uint8_t)((command_byte&~0x22u)|0x02u); /* enable AUX clock and IRQ12; IRQs remain masked in CPU */
    if(!ps2_command(0x60)||!ps2_data(command_byte))return false;
    if(!mouse_command(0xF6)||!mouse_command(0xF4))return false;
    return true;
}

static const char *const normal_map[128]={
    [2]="1",[3]="2",[4]="3",[5]="4",[6]="5",[7]="6",[8]="7",[9]="8",[10]="9",[11]="0",[12]="-",[13]="=",[14]="\b",[15]="\t",
    [16]="q",[17]="w",[18]="e",[19]="r",[20]="t",[21]="y",[22]="u",[23]="i",[24]="o",[25]="p",[26]="[",[27]="]",[28]="\n",
    [30]="a",[31]="s",[32]="d",[33]="f",[34]="g",[35]="h",[36]="j",[37]="k",[38]="l",[39]=";",[40]="'",[41]="`",[43]="\\",
    [44]="z",[45]="x",[46]="c",[47]="v",[48]="b",[49]="n",[50]="m",[51]=",",[52]=".",[53]="/",[57]=" "
};
static const char *const shifted_map[128]={
    [2]="!",[3]="@",[4]="#",[5]="$",[6]="%",[7]="^",[8]="&",[9]="*",[10]="(",[11]=")",[12]="_",[13]="+",[14]="\b",[15]="\t",
    [16]="Q",[17]="W",[18]="E",[19]="R",[20]="T",[21]="Y",[22]="U",[23]="I",[24]="O",[25]="P",[26]="{",[27]="}",[28]="\n",
    [30]="A",[31]="S",[32]="D",[33]="F",[34]="G",[35]="H",[36]="J",[37]="K",[38]="L",[39]=":",[40]="\"",[41]="~",[43]="|",
    [44]="Z",[45]="X",[46]="C",[47]="V",[48]="B",[49]="N",[50]="M",[51]="<",[52]=">",[53]="?",[57]=" "
};
static int keyboard_scancode(uint8_t code)
{
    if(code&0x80u){uint8_t key=code&0x7fu;if(key==42||key==54)g_shift=false;return -1;}
    if(code==42||code==54){g_shift=true;return -1;}if(code==58){g_caps_lock=!g_caps_lock;return -1;}
    if(code<128){const char *s=(g_shift^g_caps_lock?shifted_map:normal_map)[code];if(s)return (unsigned char)s[0];}
    return -1;
}
static void mouse_byte(uint8_t value)
{
    if(g_mouse_packet_index==0&&(value&0x08u)==0)return;
    g_mouse_packet[g_mouse_packet_index++]=value;
    if(g_mouse_packet_index<3)return;
    g_mouse_packet_index=0;
    int32_t dx=(int8_t)g_mouse_packet[1],dy=(int8_t)g_mouse_packet[2];
    if(g_mouse_packet[0]&0xC0u)return; /* Discard overflow packets. */
    g_mouse_x+=dx;g_mouse_y-=dy;
    if(g_screen.width&&g_mouse_x>=(int32_t)g_screen.width)g_mouse_x=(int32_t)g_screen.width-1;
    if(g_screen.height&&g_mouse_y>=(int32_t)g_screen.height)g_mouse_y=(int32_t)g_screen.height-1;
    if(g_mouse_x<0)g_mouse_x=0;if(g_mouse_y<0)g_mouse_y=0;
    g_mouse_buttons=g_mouse_packet[0]&7u;
    screen_redraw();
}
static int poll_input(void)
{
    uint8_t status=inb(PS2_STATUS);
    if(status&1u) {
        uint8_t value=inb(PS2_DATA);
        if((status&0x20u)!=0)mouse_byte(value);
        else return keyboard_scancode(value);
    }
    if(inb(COM1+5)&1u)return inb(COM1);
    return -1;
}
static bool command_equal(const char *a,const char *b){while(*a&&*b&&*a==*b){++a;++b;}return *a=='\0'&&*b=='\0';}
static void run_command(const char *line)
{
    if(command_equal(line,"help"))console_write("Commands: help, status, systems, reboot\n");
    else if(command_equal(line,"status")){
        console_write("Kernel: x86-64 running\n");console_write(g_screen.pixels?"Display: Multiboot2 RGB framebuffer\n":"Display: serial console (no supported framebuffer)\n");
        console_write("Keyboard: PS/2 set-1 and COM1 serial\n");console_write(g_mouse_ready?"Mouse: PS/2 enabled (left click highlighted)\n":"Mouse: PS/2 unavailable\n");
        console_write("USB HID, storage, and FAT32 loading: not implemented\n");
    } else if(command_equal(line,"systems"))console_write("Game Boy, Game Boy Color, Game Boy Advance: planned\nNintendo DS / DSi: prototype plans\n");
    else if(command_equal(line,"reboot")){
        console_write("Rebooting via the PC keyboard controller...\n");for(uint32_t n=0;n<100000u&&(inb(PS2_STATUS)&2u);++n){}outb(PS2_STATUS,0xFE);for(;;)__asm__ volatile("cli; hlt");
    } else if(*line)console_write("Unknown command. Type 'help'.\n");
}

void kernel_main(uint32_t boot_magic,uint32_t multiboot_info)
{
    serial_init();
    if(boot_magic!=MB2_BOOT_MAGIC){serial_write("devOS: invalid Multiboot2 handoff\n");for(;;)__asm__ volatile("cli; hlt");}
    bool has_framebuffer=framebuffer_init(multiboot_info);
    g_line_head=HISTORY_LINES-1;new_line();
    g_mouse_ready=mouse_init();
    if(has_framebuffer)screen_redraw();
    console_write("devOS x86-64 PC kernel\nMultiboot2 boot: OK; long mode active\n");
    console_write(has_framebuffer?"Graphics: framebuffer initialized\n":"Graphics: framebuffer unavailable; use COM1 serial\n");
    console_write("Input: PS/2 keyboard and serial; ");console_write(g_mouse_ready?"PS/2 mouse enabled\n":"PS/2 mouse unavailable\n");
    console_write("Type 'help' for commands.\n> ");
    char input[LINE_CAPACITY];uint32_t length=0;
    for(;;){
        int key=poll_input();if(key<0)continue;char ch=(char)key;
        if(ch=='\r')ch='\n';
        if(ch=='\n'){console_putc('\n');input[length]='\0';run_command(input);length=0;input[0]='\0';console_write("> ");}
        else if(ch=='\b'||ch==0x7f){if(length){--length;input[length]='\0';console_putc('\b');}}
        else if((unsigned char)ch>=0x20u&&(unsigned char)ch<=0x7eu&&length<LINE_CAPACITY-1){input[length++]=ch;input[length]='\0';console_putc(ch);}
    }
}
