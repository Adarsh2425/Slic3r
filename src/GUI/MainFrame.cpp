#include <wx/accel.h>
#include <wx/utils.h> 

#include "libslic3r.h"

#include "MainFrame.hpp"
#include "misc_ui.hpp"
#include "Dialogs/AboutDialog.hpp"

namespace Slic3r { namespace GUI {

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : MainFrame(title, pos, size, nullptr) {}
MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size, std::shared_ptr<Settings> _gui_config)
        : wxFrame(NULL, wxID_ANY, title, pos, size), loaded(false),
        tabpanel(nullptr), controller(nullptr), plater(nullptr), gui_config(_gui_config), preset_editor_tabs(std::map<wxWindowID, PresetEditor*>())
{
    this->SetIcon(wxIcon(var("Slic3r_128px.png"), wxBITMAP_TYPE_PNG));        

    this->init_tabpanel();
    this->init_menubar();

    wxToolTip::SetAutoPop(TOOLTIP_TIMER);

    // initialize status bar
    this->statusbar = new ProgressStatusBar(this, -1);
    wxString welcome_text {_("Version SLIC3R_VERSION_REPLACE - Remember to check for updates at http://slic3r.org/")};
    welcome_text.Replace("SLIC3R_VERSION_REPLACE", wxString(SLIC3R_VERSION));
    this->statusbar->SetStatusText(welcome_text);
    this->SetStatusBar(this->statusbar);

    this->loaded = 1;

    // Initialize layout
    {
        wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(this->tabpanel, 1, wxEXPAND);
        sizer->SetSizeHints(this);
        this->SetSizer(sizer);
        this->Fit();
        this->SetMinSize(wxSize(760, 490));
        this->SetSize(this->GetMinSize());
        wxTheApp->SetTopWindow(this);
        gui_config->restore_window_pos(this, "main_frame");
        this->Show();
        this->Layout();
    }
    // Set up event handlers.
    this->Bind(wxEVT_CLOSE_WINDOW, [=](wxCloseEvent& e) {
        if (e.CanVeto()) {
            if (!this->plater->prompt_unsaved_changes()) {
                e.Veto();
                return;
            }
            /*
            if ($self->{controller} && $self->{controller}->printing) {
                my $confirm = Wx::MessageDialog->new($self, "You are currently printing. Do you want to stop printing and continue anyway?",
                    'Unfinished Print', wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT);
                if ($confirm->ShowModal == wxID_NO) {
                    $event->Veto;
                    return;
                }
            }

            */
            // save window size
            gui_config->save_window_pos(this, "main_frame");

            // Propagate event
            e.Skip();
        }
    });
}

/// Private initialization function for the main frame tab panel.
void MainFrame::init_tabpanel()
{
    this->tabpanel = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP);
    auto panel = this->tabpanel; 

    panel->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, ([=](wxAuiNotebookEvent& e) 
    { 
        auto tabpanel = this->tabpanel;
        // TODO: trigger processing for activation event
        if (tabpanel->GetSelection() > 1) {
            tabpanel->SetWindowStyle(tabpanel->GetWindowStyleFlag() | wxAUI_NB_CLOSE_ON_ACTIVE_TAB);
        } else if (this->gui_config->show_host == false && tabpanel->GetSelection() == 1) {
            tabpanel->SetWindowStyle(tabpanel->GetWindowStyleFlag() | wxAUI_NB_CLOSE_ON_ACTIVE_TAB);
        } else {
            tabpanel->SetWindowStyle(tabpanel->GetWindowStyleFlag() | ~wxAUI_NB_CLOSE_ON_ACTIVE_TAB);
        }
    }), panel->GetId());

    panel->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, ([=](wxAuiNotebookEvent& e) 
    {
        if (typeid(panel) == typeid(Slic3r::GUI::PresetEditor)) {
            wxDELETE(this->preset_editor_tabs[panel->GetId()]);
        }
        wxTheApp->CallAfter([=] { this->tabpanel->SetSelection(0); });
    }), panel->GetId());

    this->plater = new Slic3r::GUI::Plater(panel, _("Plater"), gui_config);
    this->controller = new Slic3r::GUI::Controller(panel, _("Controller"));

    panel->AddPage(this->plater, this->plater->GetName());
    if (this->gui_config->show_host) panel->AddPage(this->controller, this->controller->GetName());
    
}

void MainFrame::init_menubar()
{

    wxMenu* menuFile = new wxMenu();
    {
        append_menu_item(menuFile, _(L"Open STL/OBJ/AMF/3MF…"), _("Open a model"), [=](wxCommandEvent& e) { if (this->plater != nullptr) this->plater->add();}, wxID_ANY, "brick_add.png", "Ctrl+O");
    }
    
    wxMenu* menuPlater = new wxMenu();
    {
        append_menu_item(menuPlater, _(L"Arrange…"), _("Arrange models on plater"), [=](wxCommandEvent& e) { if (this->plater != nullptr) this->plater->arrange();}, wxID_ANY, "bricks.png", "Ctrl+G");
    }
    wxMenu* menuObject = new wxMenu();
    {
    }
    wxMenu* menuSettings = new wxMenu();
    {
    }
    wxMenu* menuView = new wxMenu();
    {
    }
    wxMenu* menuWindow = new wxMenu();
    {
    }
    wxMenu* menuHelp = new wxMenu();
    {
        // TODO: Reimplement config wizard
        //menuHelp->AppendSeparator();
        append_menu_item(menuHelp, _("Slic3r &Website"), _("Open the Slic3r website in your browser"), [=](wxCommandEvent& e) 
        {
            wxLaunchDefaultBrowser("http://www.slic3r.org");
        });
        append_menu_item(menuHelp, _("Check for &Updates..."), _("Check for new Slic3r versions"), [=](wxCommandEvent& e)
        {
            check_version(true);
        });
        append_menu_item(menuHelp, _("Slic3r &Manual"), _("Open the Slic3r manual in your browser"), [=](wxCommandEvent& e) 
        {
            wxLaunchDefaultBrowser("http://manual.slic3r.org/");
        });
        append_menu_item(menuHelp, _("&About Slic3r"), _("Show about dialog"), [=](wxCommandEvent& e) 
        {
            auto about = new AboutDialog(nullptr);
            about->ShowModal();
            about->Destroy();
        }, wxID_ABOUT);
        
    }

    wxMenuBar* menubar = new wxMenuBar();
    menubar->Append(menuFile, _("&File"));
    menubar->Append(menuPlater, _("&Plater"));
    menubar->Append(menuObject, _("&Object"));
    menubar->Append(menuSettings, _("&Settings"));
    menubar->Append(menuView, _("&View"));
    menubar->Append(menuWindow, _("&Window"));
    menubar->Append(menuHelp, _("&Help"));

    this->SetMenuBar(menubar);

}

}} // Namespace Slic3r::GUI
