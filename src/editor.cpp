#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
class EditorWindow;

void set_title(EditorWindow* w);
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

    Fl_Text_Editor* editor = nullptr;
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

void about_cb(Fl_Widget*, void*) {
    fl_message("FLTK Text Editor\nHarsh Pahurkar");
}

Fl_Menu_Item menu_items[] = {
    {"&File", 0, 0, 0, FL_SUBMENU},
        {"&New", FL_CTRL + 'n', (Fl_Callback*)new_cb},
        {"&Open...", FL_CTRL + 'o', (Fl_Callback*)open_cb},
        {"&Save", FL_CTRL + 's', (Fl_Callback*)save_cb},
        {"Save &As...", FL_CTRL + FL_SHIFT + 's', (Fl_Callback*)saveas_cb, 0, FL_MENU_DIVIDER},
        {"E&xit", FL_CTRL + 'q', (Fl_Callback*)quit_cb},
        {0},
    {"&About", 0, 0, 0, FL_SUBMENU},
        {"About This Editor", 0, (Fl_Callback*)about_cb},
        {0},
    {0}
};

EditorWindow::EditorWindow(int w, int h, const char* t) : Fl_Double_Window(w, h, t) {
    begin();

    auto* menu = new Fl_Menu_Bar(0, 0, w, 28);
    menu->copy(menu_items);

    editor = new Fl_Text_Editor(0, 28, w, h - 28);
    editor->buffer(textbuf);
    editor->textfont(FL_COURIER);

    end();
    resizable(editor);
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
