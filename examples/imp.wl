hidden {CHECKBOX "Show hidden" redraw_table}
filter {TEXTBOX 20 redraw_table}

layout 1x3 { * 1x1 @
	2x1 {
		{"Directory:"}
		{TEXTBOX 20 "/home/metan/"}
	}
        {}
	6x1 {* 1x0
		2x1 {
			{"Filter:"}
			&filter
		}
		{VOID 1x1}
		&hidden
		{VOID 1x1}
		{BUTTON "Cancel"}
		{BUTTON "Open"}
	}
}
