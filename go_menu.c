#include <yed/plugin.h>

#define ARGS_GO_MENU_BUFF "*go-menu", (BUFF_SPECIAL | BUFF_RD_ONLY)

void go_menu(int n_args, char **args);
void go_menu_key_handler(yed_event *event);

yed_buffer *get_or_make_buffer(char *name, int flags) {
    yed_buffer *buff;

    if ((buff = yed_get_buffer(name)) == NULL) {
        buff = yed_create_buffer(name);
    }
    buff->flags |= flags;

    return buff;
}

int yed_plugin_boot(yed_plugin *self) {
    yed_event_handler go_menu_key;

    YED_PLUG_VERSION_CHECK();

    get_or_make_buffer(ARGS_GO_MENU_BUFF);

    yed_plugin_set_command(self, "go-menu", go_menu);

    go_menu_key.kind = EVENT_KEY_PRESSED;
    go_menu_key.fn   = go_menu_key_handler;
    yed_plugin_add_event_handler(self, go_menu_key);

    if (yed_get_var("go-menu-persistent-items") == NULL) {
        yed_set_var("go-menu-persistent-items", "");
    }

    return 0;
}

void go_menu(int n_args, char **args) {
    yed_buffer                                   *buff;
    tree_it(yed_buffer_name_t, yed_buffer_ptr_t)  bit;
    int                                           row;
    int                                           i;
    char                                         *bname;
    const char                                   *pitems;
    char                                         *pitems_cpy;

    buff = get_or_make_buffer(ARGS_GO_MENU_BUFF);

    buff->flags &= ~BUFF_RD_ONLY;

    yed_buff_clear_no_undo(buff);

    row = 1;
    tree_traverse(ys->buffers, bit) {
        if (row > 1) {
            yed_buffer_add_line_no_undo(buff);
        }
        bname = tree_it_key(bit);
        for (i = 0; i < strlen(bname); i += 1) {
            yed_append_to_line_no_undo(buff, row, G(bname[i]));
        }
        row += 1;
    }

    pitems = yed_get_var("go-menu-persistent-items");
    if (pitems != NULL) {
        pitems_cpy = strdup(pitems);
        for (bname = strtok(pitems_cpy, " "); bname != NULL; bname = strtok(NULL, " ")) {
            if (row > 1) {
                yed_buffer_add_line_no_undo(buff);
            }
            for (i = 0; i < strlen(bname); i += 1) {
                yed_append_to_line_no_undo(buff, row, G(bname[i]));
            }
            row += 1;
        }
        free(pitems_cpy);
    }

    buff->flags |= BUFF_RD_ONLY;

    YEXE("special-buffer-prepare-focus", "*go-menu");
    if (ys->active_frame) {
        YEXE("buffer", "*go-menu");
    }
}

void go_menu_key_handler(yed_event *event) {
    yed_buffer *buff;
    yed_line   *line;
    char       *bname;

    buff = get_or_make_buffer(ARGS_GO_MENU_BUFF);

    if ((event->key != ENTER && event->key != CTRL_C)
    ||  ys->interactive_command
    ||  !ys->active_frame
    ||  ys->active_frame->buffer != buff) {
        return;
    }

    event->cancel = 1;

    if (event->key == ENTER) {
        line = yed_buff_get_line(buff, ys->active_frame->cursor_line);
        array_zero_term(line->chars);
        bname = array_data(line->chars);
        if (bname[0] != '*') {
            YEXE("special-buffer-prepare-jump-focus", "*go-menu");
        }
        YEXE("buffer", bname);
    } else {
        YEXE("special-buffer-prepare-unfocus", "*go-menu");
    }
}
