#include <dirent.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define APP_NAME "Keil5_disp_size_bar V0.4"
typedef struct mcu_8051_size {
    uint32_t iram_size;
    uint32_t xram_size;
    uint32_t irom_size;
    uint32_t iram_max;
    uint32_t xram_max;
    uint32_t irom_max;
} mcu_8051_size_t;
mcu_8051_size_t mcu_8051 = {0};

void print_msg(const char *tag, const char *format, ...);
FILE *find_file_in_dir_by_name(const char *dir_name, const char *file_name);
int64_t file_sreach_string_get_pos(FILE *fp, char *sub_str, int seek, uint32_t offset);
void prtint_percentage_bar(char *label, uint8_t cnt, uint32_t val, uint32_t max);
uint32_t str_get_hex(char *str, char *str_hex);
void process_execution_region(FILE *fp_map);
float str_get_dec(char *str, char *str_dec);
uint8_t find_8051(void);
void find_project_size_define(void);

uint32_t ram_max_from_porject = 0;
uint32_t rom_max_from_porject = 0;

// argv[0] 是当前exe文件的路径
// exe放在工程文件的同一目录下就可以获取到工程目录了
int main(int argc, char *argv[])
{
    DIR *dir;
    char *path = NULL;
    FILE *fp_map;
    // 打开当前目录
    int64_t pos = 0;

    if (find_8051())
        return;

    find_project_size_define();

    print_msg(APP_NAME, "find map file start");
    fp_map = find_file_in_dir_by_name("./", ".map");

    process_execution_region(fp_map);

    fclose(fp_map);
}

void print_msg(const char *tag, const char *format, ...)
{
    printf("[%s]: ", tag);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

FILE *find_file_in_dir_by_name(const char *dir_name, const char *file_name)
{
    FILE *fp;
    DIR *dir;
    struct dirent *st;
    struct stat sta;

    int ret = 0;
    char tmp_name[1024] = {0};

    if ((dir = opendir(dir_name)) == NULL) {
        printf("%s: Open dir error\n", APP_NAME);
        return NULL;
    }

    while ((st = readdir(dir)) != NULL) {
        strcpy(tmp_name, dir_name);
        strcat(tmp_name, "/");
        strcat(tmp_name, st->d_name); // 新文件路径名

        ret = stat(tmp_name, &sta); // 查看目录下文件属性

        if (ret < 0) {
            print_msg(APP_NAME "Read stat failed for %s", tmp_name);
            continue;
        }

        if (S_ISDIR(sta.st_mode)) { // 如果为目录文件
            if (strcmp("..", st->d_name) == 0 || strcmp(".", st->d_name) == 0) {
                continue; // 忽略当前目录和上一层目录
            } else {
                fp = find_file_in_dir_by_name(tmp_name, file_name); // 递归读取
                if (fp) {
                    closedir(dir);
                    return fp;
                }
            }
        } else { // 不为目录则打印文件路径名
            if (strstr(tmp_name, file_name)) {
                fp = fopen(tmp_name, "r+");
                if (!fp) {
                    // print_msg(APP_NAME, "Failed to open file: %s", tmp_name);
                    return NULL;
                } else {
                    print_msg(APP_NAME, "Found file: %s", tmp_name);
                    closedir(dir);
                    return fp;
                }
            }
        }
    }

    closedir(dir);
    return NULL;
}

int64_t file_sreach_string_get_pos(FILE *fp, char *sub_str, int seek, uint32_t offset)
{
    char str[1000];
    int64_t pos = 0;
    fseek(fp, offset, seek);

    while (NULL != fgets(str, 1000, fp)) {

        if (strstr(str, sub_str)) {

            // printf("%s", str);
            pos = ftell(fp) - strlen(str);
            fseek(fp, pos, SEEK_SET);
            return pos;
        }
    }
    return -1;
}

void prtint_percentage_bar(char *label, uint8_t cnt, uint32_t val, uint32_t max)
{
    uint8_t i;
    printf("%5d KB %5s%u:|", (uint32_t)(max / 1024.0f), label, cnt);

    for (i = 0; i < 20; i++) {
        if (i < (uint8_t)((val / 1.0) / (max / 1.0) * 20)) {

            printf("■");
        } else {
            printf("_");
        }
    }
    printf("|%6.2f %% (%8.2f KB / %8.2f KB)", (val / 1.0) / (max / 1.0) * 100, val / 1024.0f, max / 1024.0f);
    printf(" [%u B]\n", max - val);
}

uint32_t str_get_hex(char *str, char *str_hex)
{

    char *str_buf;
    uint32_t hex;

    str_buf = strstr(str, str_hex);
    if (str_buf == NULL)
        return 0;
    str_buf += strlen(str_hex);
    sscanf(str_buf, "%x", &hex);

    // printf("%s:%#x\n", str_hex,hex);

    return hex;
}

float str_get_dec(char *str, char *str_dec)
{

    char *str_buf;
    float dec;

    str_buf = strstr(str, str_dec);
    if (str_buf == NULL)
        return 0;
    str_buf += strlen(str_dec);
    sscanf(str_buf, "%f", &dec);

    // printf("%s:%#x\n", str_hex,hex);

    return dec;
}

void process_execution_region(FILE *fp_map)
{

    int64_t pos = 0;
    char str[1000];
    char *ch_p = NULL;
    uint32_t exec_base = 0, load_base = 0, size = 0, max = 0;
    uint8_t ram_cnt = 0, flash_cnt = 0;

    uint8_t cnt = 0;
    uint32_t ram_size[30];
    uint32_t flash_size[30];

    uint32_t ram_max[30];
    uint32_t flash_max[30];

    while (1) {

        pos = file_sreach_string_get_pos(fp_map, "Execution Region", SEEK_SET, pos);
        if (-1 == pos) {
            print_msg(APP_NAME, "map file read end\n");
            break;
        }

        fseek(fp_map, pos, SEEK_SET);
        fgets(str, 1000, fp_map);

        if (strstr(str, "RAM")) {

            ram_size[ram_cnt] = str_get_hex(str, "Size: ");
            pos += strstr(str, "Size: ") - str;
            fseek(fp_map, pos, SEEK_SET);

            ram_max[ram_cnt] = str_get_hex(str, "Max: ");
            if (ram_max[ram_cnt] > 100 * 1024 * 1024)
                ram_max[ram_cnt] = ram_max_from_porject;

            pos += strstr(str, "Max: ") - str;
            fseek(fp_map, pos, SEEK_SET);
            if (ram_cnt < 30)
                ram_cnt++;
            // prtint_percentage_bar("ram",ram_cnt, size, max);
        } else if (strstr(str, "ER$$") || strstr(str, "ER_RW") || strstr(str, "ER_ZI")) {

            ram_size[ram_cnt] = str_get_hex(str, "Size: ");
            pos += strstr(str, "Size: ") - str;
            fseek(fp_map, pos, SEEK_SET);

            ram_max[ram_cnt] = str_get_hex(str, "Max: ");
            if (ram_max[ram_cnt] > 100 * 1024 * 1024)
                ram_max[ram_cnt] = ram_max_from_porject;
            print_msg(APP_NAME, "RAMMAX:%d\n", ram_max[ram_cnt]);

            pos += strstr(str, "Max: ") - str;
            fseek(fp_map, pos, SEEK_SET);
            if (ram_cnt < 30)
                ram_cnt++;
            // prtint_percentage_bar("ram",ram_cnt, size, max);
        } else {

            flash_size[flash_cnt] = str_get_hex(str, "Size: ");
            pos += strstr(str, "Size: ") - str;
            fseek(fp_map, pos, SEEK_SET);

            flash_max[flash_cnt] = str_get_hex(str, "Max: ");
            if (flash_max[flash_cnt] > 100 * 1024 * 1024)
                flash_max[flash_cnt] = rom_max_from_porject;

            pos += strstr(str, "Max: ") - str;
            fseek(fp_map, pos, SEEK_SET);
            if (flash_cnt < 30)
                flash_cnt++;
            // prtint_percentage_bar("flash",flash_cnt, size, max);
        }
    }

    printf("ram:\n");
    for (cnt = 0; cnt < ram_cnt; cnt++) {
        prtint_percentage_bar("ram", cnt + 1, ram_size[cnt], ram_max[cnt]);
    }
    printf("flash:\n");
    for (cnt = 0; cnt < flash_cnt; cnt++) {
        prtint_percentage_bar("flash", cnt + 1, flash_size[cnt], flash_max[cnt]);
    }

    printf(" \n");
}

uint8_t find_8051(void)
{
    FILE *fp_project = NULL;
    FILE *fp_m51 = NULL;
    int64_t pos = 0;
    char str[1000];

    uint8_t cnt = 0;

    print_msg(APP_NAME, "find project satrt!");
    fp_project = find_file_in_dir_by_name("./", ".uvproj");
    if (fp_project == NULL) {
        print_msg(APP_NAME, "find project fail!");
        fp_project = find_file_in_dir_by_name("./", ".uvprojx");
        if (fp_project == NULL) {
            print_msg(APP_NAME, "find project fail!");
            return 0;
        }
    }

    while (1) {

        pos = file_sreach_string_get_pos(fp_project, "<Cpu>", SEEK_SET, pos);
        if (-1 == pos) {
            print_msg(APP_NAME, "found cup info fail\n");
            return 0;
        }

        fseek(fp_project, pos - 8, SEEK_SET);
        fgets(str, 1000, fp_project);
        if (strstr(str, "IRAM(0-")) {

            print_msg(APP_NAME, "cup is 8051\n");

            mcu_8051.iram_max = str_get_hex(str, "IRAM(0-");

            if (strstr(str, "XRAM(0-"))
                mcu_8051.xram_max = str_get_hex(str, "XRAM(0-");
            else
                mcu_8051.xram_max = mcu_8051.iram_max;

            mcu_8051.irom_max = str_get_hex(str, "IROM(0-");
            // printf("iram_max:%d,xram_max:%d,irom_max:%d\n",mcu_8051.iram_max,mcu_8051.xram_max,mcu_8051.irom_max);
            fclose(fp_project); // 关闭文件
            fp_m51 = find_file_in_dir_by_name("./", ".m51");
            pos = file_sreach_string_get_pos(fp_m51, "Program Size:", SEEK_SET, pos);
            fgets(str, 1000, fp_project);
            mcu_8051.iram_size = str_get_dec(str, "data=");

            mcu_8051.irom_size = str_get_dec(str, "code=");
            // printf("iram_size:%d,xram_size:%d,irom_size:%d\n",mcu_8051.iram_size,mcu_8051.xram_size,mcu_8051.irom_size);

            prtint_percentage_bar("flash", 1, mcu_8051.irom_size, mcu_8051.irom_max);
            prtint_percentage_bar("iram", 1, mcu_8051.iram_size, mcu_8051.iram_max);

            if (strstr(str, "xdata=")) {
                mcu_8051.xram_size = str_get_dec(str, "xdata=");
                prtint_percentage_bar("xram", 1, mcu_8051.xram_size, mcu_8051.xram_max);
            }

            return 1;

        } else {
            return 0;
        }
    }

    return 0;
}

void find_project_size_define(void)
{

    FILE *fp_project = NULL;
    int64_t pos = 0;
    char str[1000];
    char *ch_p = NULL;

    // print_msg(APP_NAME, "find project satrt!");
    fp_project = find_file_in_dir_by_name("./", ".uvproj");
    if (fp_project == NULL) {
        fp_project = find_file_in_dir_by_name("./", ".uvprojx");
        if (fp_project == NULL) {
            // print_msg(APP_NAME, "find project fail!");
            return;
        }
    }

    while (1) {

        pos = file_sreach_string_get_pos(fp_project, "<Cpu>", SEEK_SET, pos);
        if (-1 == pos) {
            print_msg(APP_NAME, "found cup info fail\n");
            return;
        }

        fseek(fp_project, pos - 8, SEEK_SET);
        fgets(str, 1000, fp_project);
        if (ch_p = strstr(str, "IRAM(")) {
            if (strstr(ch_p, ",")) {
                ram_max_from_porject = str_get_hex(ch_p, ",");
                ch_p = strstr(ch_p, ",");
                ch_p++;
                rom_max_from_porject = str_get_hex(ch_p, ",");
            } else if (strstr(ch_p, "-")) {
                ram_max_from_porject = str_get_hex(ch_p, "-");
                ch_p = strstr(ch_p, "-");
                ch_p++;
                rom_max_from_porject = str_get_hex(ch_p, "-");
            }
            // print_msg(APP_NAME,"IARM:%d,IROM:%d\n",ram_max_from_porject,rom_max_from_porject);
            return;
        } else {
            return;
        }
    }

    fclose(fp_project); // 关闭文件
}