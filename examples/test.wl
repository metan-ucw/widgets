# Static test app layout
layout 1x6 { @
    {"Heading" bold}
    {"--- Hello World! ---"}
    {CHECKBOX "Check Box" NULL 1}
    {TEXTBOX 14 ["0123456789"] textbox_filter}
    {RADIOBUTTON "Fist" "Second" radiobutton_callback}
    3x1 { * 1x0
        {BUTTON "Cancel" btn_cancel_callback}
        {VOID 1x0}
        {BUTTON "Ok" btn_ok_callback}
    }
}
