#include <yed/plugin.h>
#include <time.h>

#define ARGS_GO_MENU_BUFF "*go-menu", (BUFF_SPECIAL | BUFF_RD_ONLY)

void go_menu(int n_args, char **args);
void go_menu_key_handler(yed_event *event);
void go_menu_line_handler(yed_event *event);
void go_menu_frame_handler(yed_event *event);
int  _go_menu_if_modified(char *path);

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
    yed_event_handler go_menu_line;
    yed_event_handler go_menu_frame;

    YED_PLUG_VERSION_CHECK();

    get_or_make_buffer(ARGS_GO_MENU_BUFF);

    yed_plugin_set_command(self, "go-menu", go_menu);

    go_menu_key.kind = EVENT_KEY_PRESSED;
    go_menu_key.fn   = go_menu_key_handler;
    yed_plugin_add_event_handler(self, go_menu_key);

    go_menu_line.kind = EVENT_LINE_PRE_DRAW;
    go_menu_line.fn   = go_menu_line_handler;
    yed_plugin_add_event_handler(self, go_menu_line);

    go_menu_frame.kind = EVENT_FRAME_PRE_UPDATE;
    go_menu_frame.fn   = go_menu_frame_handler;
    yed_plugin_add_event_handler(self, go_menu_frame);

    if (yed_get_var("go-menu-persistent-items") == NULL) {
        yed_set_var("go-menu-persistent-items", "");
    }

    if (yed_get_var("go-menu-modified") == NULL) {
        yed_set_var("go-menu-modified", "Yes");
    }

    return 0;
}

void go_menu(int n_args, char **args) {
    array_t                                        pitems_array;
    const char                                    *pitems;
    char                                          *pitems_cpy;
    char                                          *bname;
    char                                         **nameit;
    yed_buffer                                    *buff;
    tree_it(yed_buffer_name_t, yed_buffer_ptr_t)   bit;
    int                                            row;
    int                                            i;

    buff = get_or_make_buffer(ARGS_GO_MENU_BUFF);

    buff->flags &= ~BUFF_RD_ONLY;

    yed_buff_clear_no_undo(buff);

    pitems_array = array_make(char*);

    pitems = yed_get_var("go-menu-persistent-items");
    if (pitems != NULL) {
        pitems_cpy = strdup(pitems);
        for (bname = strtok(pitems_cpy, " "); bname != NULL; bname = strtok(NULL, " ")) {
            bname = strdup(bname);
            array_push(pitems_array, bname);
        }
        free(pitems_cpy);
    }

    row = 1;
    tree_traverse(ys->buffers, bit) {
        bname = tree_it_key(bit);

        array_traverse(pitems_array, nameit) {
            if (strcmp(bname, *nameit) == 0) { goto next; }
        }

        if (row > 1) {
            yed_buffer_add_line_no_undo(buff);
        }
        for (i = 0; i < strlen(bname); i += 1) {
            yed_append_to_line_no_undo(buff, row, G(bname[i]));
        }
        row += 1;
next:;
    }

    array_traverse(pitems_array, nameit) {
        bname = *nameit;
        if (row > 1) {
            yed_buffer_add_line_no_undo(buff);
        }
        for (i = 0; i < strlen(bname); i += 1) {
            yed_append_to_line_no_undo(buff, row, G(bname[i]));
        }
        row += 1;
    }


    free_string_array(pitems_array);

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
    char       *bname_cpy;

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

        if (yed_var_is_truthy("go-menu-modified") && strstr(bname, " <modified>")) {
            bname_cpy = strdup(bname);
            bname_cpy[strlen(bname_cpy) - 11] = '\0';
            YEXE("buffer", bname_cpy);
            free(bname);
        }else {
            YEXE("buffer", bname);
        }
    } else {
        YEXE("special-buffer-prepare-unfocus", "*go-menu");
    }
}

void go_menu_line_handler(yed_event *event) {
    yed_frame *frame;
    yed_attrs *ait;
    char      *s;
    yed_attrs  attr;
    yed_attrs  attr_m;
    char      *pitems;
    char      *pitems_cpy;
    char      *bname;
    char      *bname_cpy;
    int        modified;
    int        loc;

    if (event->frame         == NULL
    ||  event->frame->buffer == NULL
    ||  event->frame->buffer != get_or_make_buffer(ARGS_GO_MENU_BUFF)) {
        return;
    }

    frame = event->frame;

    s = yed_get_line_text(frame->buffer, event->row);
    if (s == NULL) { return; }

    attr   = ZERO_ATTR;
    attr_m = ZERO_ATTR;

    attr_m = yed_active_style_get_attention();

    if (strlen(s) > 0 && *s == '*') {
        attr = yed_active_style_get_code_constant();
    }

    pitems = yed_get_var("go-menu-persistent-items");
    if (pitems != NULL) {
        pitems_cpy = strdup(pitems);
        for (bname = strtok(pitems_cpy, " "); bname != NULL; bname = strtok(NULL, " ")) {
            if (strcmp(bname, s) == 0) {
                attr = yed_active_style_get_code_keyword();
                break;
            }
        }
        free(pitems_cpy);
    }

    attr.flags &= ~ATTR_BOLD;
    if (yed_get_buffer(s) != NULL) {
        attr.flags |= ATTR_BOLD;
    }

    modified = 0;
    if (yed_var_is_truthy("go-menu-modified") && strstr(s, " <modified>")) modified = 1;

    loc = 0;
    array_traverse(event->line_attrs, ait) {
        if (modified == 1 && loc >= (strlen(s) - 11)) {
            yed_combine_attrs(ait, &attr_m);
        }else{
            yed_combine_attrs(ait, &attr);
        }
        loc++;
    }

    free(s);
}

void go_menu_frame_handler(yed_event *event) {
    if (!yed_var_is_truthy("go-menu-modified")) { return; }

    yed_frame *frame;
    char      *s;
    char      *bname;

    if (event->frame         == NULL
    ||  event->frame->buffer == NULL
    ||  event->frame->buffer != get_or_make_buffer(ARGS_GO_MENU_BUFF)) {
        return;
    }

    frame = event->frame;

    for (int i = 1; i < yed_buff_n_lines(frame->buffer); i++) {
        s = yed_get_line_text(frame->buffer, i);

        if (s == NULL) { return; }

        bname = strdup(s);
        if (strstr(bname, " <modified>")) {
            bname[strlen(bname) - 11] = '\0';
        }

        if (_go_menu_if_modified(bname)) {
            bname = strcat(bname, " <modified>");
        }

        frame->buffer->flags &= !BUFF_RD_ONLY;
        yed_line_clear_no_undo(frame->buffer, i);
        yed_buff_insert_string_no_undo(frame->buffer, bname, i, 1);
        frame->buffer->flags &= BUFF_RD_ONLY;

        free(bname);
        free(s);
    }
}

int _go_menu_if_modified(char *path) {
    yed_buffer *buff;

    buff = yed_get_buffer(path);
    if (buff != NULL) {
        if ((buff->flags & BUFF_MODIFIED)  &&
           !(buff->flags & BUFF_SPECIAL)) {
            return 1;
        }
    }
    return 0;
}
