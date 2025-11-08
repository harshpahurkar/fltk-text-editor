#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Box.H>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {
class EditorWindow;

void set_title(EditorWindow* w);
void update_status(EditorWindow* w);
void load_file(const char* path, int insert_pos);
void save_file(const char* path);

Fl_Text_Buffer* textbuf = nullptr;
int changed = 0;
int loading = 0;
char filename[512] = "";
char title[512] = "";
EditorWindow* main_window = nullptr;

class EditorWindow : public Fl_Double_Window {
public:
    EditorWindow(int w, int h, const char* t);
    ~EditorWindow() override = default;
    void resize(int X, int Y, int W, int H) override;

    Fl_Window* replace_dlg = nullptr;
    Fl_Input* replace_find = nullptr;
    Fl_Input* replace_with = nullptr;
    Fl_Button* replace_all = nullptr;
    Fl_Return_Button* replace_next = nullptr;
    Fl_Button* replace_cancel = nullptr;

    Fl_Text_Editor* editor = nullptr;
    Fl_Box* status = nullptr;
    char search[256] = "";
};

void set_title(EditorWindow* w) {
    if (filename[0] == '\0') {
        std::strcpy(title, "Untitled");
    } else {
        const char* slash = std::strrchr(filename, '/');
#ifdef _WIN32
        if (!slash) slash = std::strrchr(filename, '\\');
#endif
        if (slash) std::strcpy(title, slash + 1);
        else std::strcpy(title, filename);
    }

    if (changed) std::strcat(title, " (modified)");
    w->label(title);
}

void update_status(EditorWindow* w) {
    if (!w || !w->status || !textbuf) return;
    int pos = w->editor->insert_position();
    int line = textbuf->count_lines(0, pos) + 1;
    int line_start = textbuf->line_start(pos);
    int col = pos - line_start + 1;

    std::string text = textbuf->text();
    int words = 0;
    bool in_word = false;
    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            if (!in_word) words++;
            in_word = true;
        } else {
            in_word = false;
        }
    }

    char buf[128];
    std::snprintf(buf, sizeof(buf), "Ln %d, Col %d | Words %d", line, col, words);
    w->status->label(buf);
}

void status_timer_cb(void* v) {
    auto* w = static_cast<EditorWindow*>(v);
    update_status(w);
    Fl::repeat_timeout(0.25, status_timer_cb, v);
}

int check_save() {
    if (!changed) return 1;
    int choice = fl_choice(
        "The current document has changes.\nSave them?",
        "Cancel", "Save", "Discard"
    );

    if (choice == 1) {
        const char* path = filename[0] == '\0'
            ? fl_file_chooser("Save File", "*", filename)
            : filename;
        if (!path) return 0;
        save_file(path);
        return !changed;
    }

    return choice == 2;
}

void load_file(const char* path, int insert_pos) {
    loading = 1;
    const bool insert = (insert_pos != -1);
    if (!insert) filename[0] = '\0';

    int result = insert ? textbuf->insertfile(path, insert_pos)
                        : textbuf->loadfile(path);

    if (result) {
        fl_alert("Failed to read file: %s", path);
    } else if (!insert) {
        std::strcpy(filename, path);
    }

    loading = 0;
    changed = insert ? 1 : 0;
    textbuf->call_modify_callbacks();
}

void save_file(const char* path) {
    if (textbuf->savefile(path)) {
        fl_alert("Failed to write file: %s", path);
        return;
    }
    std::strcpy(filename, path);
    changed = 0;
    textbuf->call_modify_callbacks();
}

void changed_cb(int, int inserted, int deleted, int, const char*, void* v) {
    if ((inserted || deleted) && !loading) changed = 1;
    auto* w = static_cast<EditorWindow*>(v);
    set_title(w);
    update_status(w);
    if (loading) w->editor->show_insert_position();
}

void new_cb(Fl_Widget*, void*) {
    if (!check_save()) return;
    filename[0] = '\0';
    textbuf->select(0, textbuf->length());
    textbuf->remove_selection();
    changed = 0;
    textbuf->call_modify_callbacks();
}

void open_cb(Fl_Widget*, void*) {
    if (!check_save()) return;
    const char* path = fl_file_chooser("Open File", "*", filename);
    if (path) load_file(path, -1);
}

void insert_cb(Fl_Widget*, void*) {
    const char* path = fl_file_chooser("Insert File", "*", filename);
    if (path && main_window) {
        load_file(path, main_window->editor->insert_position());
    }
}

void save_cb(Fl_Widget*, void*) {
    if (filename[0] == '\0') {
        const char* path = fl_file_chooser("Save File As", "*", filename);
        if (path) save_file(path);
        return;
    }
    save_file(filename);
}

void saveas_cb(Fl_Widget*, void*) {
    const char* path = fl_file_chooser("Save File As", "*", filename);
    if (path) save_file(path);
}

void quit_cb(Fl_Widget*, void*) {
    if (!check_save()) return;
    std::exit(0);
}

void copy_cb(Fl_Widget*, void* v) {
    if (main_window) Fl_Text_Editor::kf_copy(0, main_window->editor);
}

void cut_cb(Fl_Widget*, void* v) {
    if (main_window) Fl_Text_Editor::kf_cut(0, main_window->editor);
}

void paste_cb(Fl_Widget*, void* v) {
    if (main_window) Fl_Text_Editor::kf_paste(0, main_window->editor);
}

void delete_cb(Fl_Widget*, void*) {
    textbuf->remove_selection();
}

void find_next(Fl_Widget*, void* v) {
    auto* w = main_window;
    if (!w) return;
    if (w->search[0] == '\0') return;

    int pos = w->editor->insert_position();
    int found = textbuf->search_forward(pos, w->search, &pos);
    if (found) {
        textbuf->select(pos, pos + std::strlen(w->search));
        w->editor->insert_position(pos + std::strlen(w->search));
        w->editor->show_insert_position();
        update_status(w);
    } else {
        fl_alert("No more matches for '%s'", w->search);
    }
}

void find_cb(Fl_Widget* w, void* v) {
    auto* e = main_window;
    if (!e) return;
    const char* val = fl_input("Find:", e->search);
    if (val) {
        std::strncpy(e->search, val, sizeof(e->search) - 1);
        e->search[sizeof(e->search) - 1] = '\0';
        find_next(w, v);
    }
}

void replace_cb(Fl_Widget*, void* v) {
    auto* e = main_window;
    if (!e) return;
    e->replace_dlg->show();
}

void replace_next_cb(Fl_Widget*, void* v) {
    auto* e = main_window;
    if (!e) return;
    const char* find = e->replace_find->value();
    const char* repl = e->replace_with->value();

    if (!find || find[0] == '\0') {
        e->replace_dlg->show();
        return;
    }

    int pos = e->editor->insert_position();
    int found = textbuf->search_forward(pos, find, &pos);
    if (found) {
        textbuf->select(pos, pos + std::strlen(find));
        textbuf->remove_selection();
        textbuf->insert(pos, repl);
        e->editor->insert_position(pos + std::strlen(repl));
        e->editor->show_insert_position();
        update_status(e);
    } else {
        fl_alert("No match for '%s'", find);
    }
}

void replace_all_cb(Fl_Widget*, void* v) {
    auto* e = main_window;
    if (!e) return;
    const char* find = e->replace_find->value();
    const char* repl = e->replace_with->value();

    if (!find || find[0] == '\0') {
        e->replace_dlg->show();
        return;
    }

    int times = 0;
    e->editor->insert_position(0);
    for (;;) {
        int pos = e->editor->insert_position();
        int found = textbuf->search_forward(pos, find, &pos);
        if (!found) break;
        textbuf->select(pos, pos + std::strlen(find));
        textbuf->remove_selection();
        textbuf->insert(pos, repl);
        e->editor->insert_position(pos + std::strlen(repl));
        times++;
    }

    if (times > 0) fl_message("Replaced %d occurrence(s).", times);
    else fl_alert("No match for '%s'", find);
    update_status(e);
}

void replace_cancel_cb(Fl_Widget*, void* v) {
    auto* e = main_window;
    if (!e) return;
    e->replace_dlg->hide();
}

void about_cb(Fl_Widget*, void*) {
    fl_message("FLTK Text Editor\nHarsh Pahurkar\nMinimal, fast, recruiter-ready.");
}

Fl_Menu_Item menu_items[] = {
    {"&File", 0, 0, 0, FL_SUBMENU},
        {"&New", FL_CTRL + 'n', (Fl_Callback*)new_cb},
        {"&Open...", FL_CTRL + 'o', (Fl_Callback*)open_cb},
        {"&Insert...", FL_CTRL + 'i', (Fl_Callback*)insert_cb, 0, FL_MENU_DIVIDER},
        {"&Save", FL_CTRL + 's', (Fl_Callback*)save_cb},
        {"Save &As...", FL_CTRL + FL_SHIFT + 's', (Fl_Callback*)saveas_cb, 0, FL_MENU_DIVIDER},
        {"E&xit", FL_CTRL + 'q', (Fl_Callback*)quit_cb},
        {0},
    {"&About", 0, 0, 0, FL_SUBMENU},
        {"About This Editor", 0, (Fl_Callback*)about_cb},
        {0},
    {"&Edit", 0, 0, 0, FL_SUBMENU},
        {"Cu&t", FL_CTRL + 'x', (Fl_Callback*)cut_cb},
        {"&Copy", FL_CTRL + 'c', (Fl_Callback*)copy_cb},
        {"&Paste", FL_CTRL + 'v', (Fl_Callback*)paste_cb},
        {"&Delete", 0, (Fl_Callback*)delete_cb},
        {0},
    {"&Search", 0, 0, 0, FL_SUBMENU},
        {"&Find...", FL_CTRL + 'f', (Fl_Callback*)find_cb},
        {"Find &Next", FL_CTRL + 'g', (Fl_Callback*)find_next},
        {"&Replace...", FL_CTRL + 'r', (Fl_Callback*)replace_cb},
        {"Replace &Next", FL_CTRL + 't', (Fl_Callback*)replace_next_cb},
        {"Replace &All", FL_CTRL + 'a', (Fl_Callback*)replace_all_cb},
        {0},
    {0}
};

EditorWindow::EditorWindow(int w, int h, const char* t) : Fl_Double_Window(w, h, t) {
    begin();

    auto* menu = new Fl_Menu_Bar(0, 0, w, 28);
    menu->copy(menu_items);

    editor = new Fl_Text_Editor(0, 28, w, h - 50);
    editor->buffer(textbuf);
    editor->textfont(FL_COURIER);

    status = new Fl_Box(0, h - 22, w, 22);
    status->box(FL_FLAT_BOX);
    status->labelfont(FL_COURIER);
    status->labelsize(12);
    status->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    replace_dlg = new Fl_Window(320, 120, "Replace");
    replace_find = new Fl_Input(80, 10, 220, 25, "Find:");
    replace_with = new Fl_Input(80, 45, 220, 25, "Replace:");
    replace_all = new Fl_Button(10, 80, 90, 25, "Replace All");
    replace_next = new Fl_Return_Button(110, 80, 110, 25, "Replace Next");
    replace_cancel = new Fl_Button(230, 80, 80, 25, "Cancel");

    replace_all->callback(replace_all_cb, this);
    replace_next->callback(replace_next_cb, this);
    replace_cancel->callback(replace_cancel_cb, this);

    replace_dlg->set_non_modal();
    replace_dlg->end();

    end();
    resizable(editor);
    update_status(this);
    Fl::add_timeout(0.25, status_timer_cb, this);
}

void EditorWindow::resize(int X, int Y, int W, int H) {
    Fl_Double_Window::resize(X, Y, W, H);
    if (editor) editor->resize(0, 28, W, H - 50);
    if (status) status->resize(0, H - 22, W, 22);
}
} // namespace

int main(int argc, char** argv) {
    textbuf = new Fl_Text_Buffer();

    auto* window = new EditorWindow(800, 600, "Editor");
    main_window = window;
    textbuf->add_modify_callback(changed_cb, window);
    textbuf->call_modify_callbacks();

    window->show(argc, argv);

    if (argc > 1) {
        load_file(argv[1], -1);
    }

    return Fl::run();
}