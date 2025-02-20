#include "cat_utility.h"

uint16_t get_options(int argc, char* argv[]) {
  int opt;
  uint16_t flags = 0;

  while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) !=
         -1) {
    switch (opt) {
      case 'b':
        flags = set_option(number_nonblank, flags, 1);
        break;
      case 'e':
        flags = set_option(show_ends, flags, 1);
        flags = set_option(show_nonprinting, flags, 1);
        break;
      case 'v':
        flags = set_option(show_nonprinting, flags, 1);
        break;
      case 'n':
        flags = set_option(number, flags, 1);
        break;
      case 's':
        flags = set_option(squeeze_blank, flags, 1);
        break;
      case 't':
        flags = set_option(show_tabs, flags, 1);
        flags = set_option(show_nonprinting, flags, 1);
        break;
      case 'E':
        flags = set_option(show_ends, flags, 1);
        break;
      case 'T':
        flags = set_option(show_tabs, flags, 1);
        break;
      case '?':
        err_quit(SYNOPSIS);
    }
  }

  if (optind < argc) {
    flags = set_option(is_file, flags, 1);
  }
  return flags;
}

void exec_options(uint16_t flags, FILE* fp) {
  _Bool flag_numreset = 1;
  int ch;

  while ((ch = fgetc(fp)) != EOF) {
    if (get_option(squeeze_blank, flags)) {
      if (exec_squeeze_blank(ch, fp)) {
        continue;
      }
    }
    if (get_option(number_nonblank, flags)) {
      flags = set_option(number, flags, 0);
      if (exec_number_nonblank(flag_numreset, ch, fp)) {
        flag_numreset = 0;
        continue;
      }
      flag_numreset = 0;
    }
    if (get_option(number, flags)) {
      if (exec_number(flag_numreset, ch, fp)) {
        flag_numreset = 0;
        continue;
      }
      flag_numreset = 0;
    }
    if (get_option(show_nonprinting, flags)) {
      ch = exec_nonprinting(ch);
    }
    if (get_option(show_tabs, flags)) {
      if (ch == '\t') {
        printf("^I");
        continue;
      }
    }
    if (ch != '\n') {
      printf("%c", ch);
    }
    if (get_option(show_ends, flags)) {
      if (ch == '\n') {
        printf("$");
      }
    }
    if (ch == '\n') {
      printf("\n");
    }
  }
}

_Bool get_option(uint16_t option, uint16_t flags) {
  return (flags & option) == option;
}

uint16_t set_option(uint16_t target, uint16_t flags, _Bool status) {
  if (status == 1) {
    flags |= target;
  } else {
    flags &= ~target;
  }
  return flags;
}

void simple_cat(uint16_t flags) {
  int n = 0;
  int buf[maxline];

  exec_options(flags, stdin);
  while ((n = read(STDIN_FILENO, buf, maxline)) != 0) {
    exec_options(flags, stdin);
    if (write(STDOUT_FILENO, buf, n) != n) {
      err_sys("input error");
    }
  }
  if (n == -1) {
    err_sys("read error");
  }
}

_Bool exec_squeeze_blank(char ch, FILE* fp) {
  static _Bool is_prevsymbol = 0;
  char next_char;
  _Bool contin = 0;

  if ((next_char = fgetc(fp)) != EOF) {
    contin = (is_prevsymbol == 0 && ch == '\n' && next_char == '\n');
    ungetc(next_char, fp);
  }
  is_prevsymbol = (ch != '\n');
  return contin;
}

_Bool exec_number_nonblank(_Bool reset_flag, char ch, FILE* fp) {
  static uint32_t line_count = 0;
  _Bool contin = 0;
  char next_char;

  if (reset_flag) {
    line_count = 0;
  }
  if (line_count == 0 && ch != '\n') {
    ++line_count;
    printf("%6d\t", line_count);
  }
  if (ch == '\n') {
    if ((next_char = fgetc(fp)) != EOF) {
      if (next_char != '\n') {
        ++line_count;
        contin = 1;
        printf("\n%6d\t", line_count);
      }

      ungetc(next_char, fp);
    }
  }
  return contin;
}

_Bool exec_number(_Bool reset_flag, char ch, FILE* fp) {
  static uint32_t line_count = 0;
  _Bool contin = 0;

  if (reset_flag) {
    line_count = 0;
  }
  if (line_count == 0 && ch != '\n') {
    ++line_count;
    printf("%6d\t", line_count);
  }
  int next_ch;
  if (ch == '\n') {
    ++line_count;
    if (line_count == 1) {
      printf("%6d\t\n", line_count);
      ++line_count;
      if ((next_ch = fgetc(fp)) != EOF) {
        ungetc(next_ch, fp);
        printf("%6d\t", line_count);
      }
    } else {
      if ((next_ch = fgetc(fp)) == EOF) {
        printf("\n");
      } else {
        ungetc(next_ch, fp);
        printf("\n%6d\t", line_count);
      }
    }
    contin = 1;
  }

  return contin;
}

char exec_nonprinting(char ch) {
  if ((ch >= 0 && ch < 9) || (ch > 10 && ch <= 31)) {
    printf("^");
    ch += '@';
  }
  if (ch == 127) {
    printf("^");
    ch = '?';
  }
  if (ch <= -97) {
    printf("M-^");
    ch += 128 + '@';
  }
#ifdef __linux__
  if (ch >= -96 && ch <= -2) {
    printf("M-");
    ch += 96 + ' ';
  }
  if (ch == -1) {
    printf("M-^?");
  }
#endif  // __linux__
  return ch;
}
