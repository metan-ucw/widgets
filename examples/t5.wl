# Static test app layout
layout 1x3 { @
    {BUTTON "Button 0" callback}
    "First" "Second" {
	2x2 {
	    {BUTTON "Button 1" callback}
	    {BUTTON "Button 2" callback}
	    {BUTTON "Button 3" callback}
	    {BUTTON "Button 4" callback}
	}
	2x1 {
	    1x2 {
	        {BUTTON "Button 1" callback}
	        {BUTTON "Button 2" callback}
	    }
	    1x2 {
	        {BUTTON "Button 3" callback}
	        {BUTTON "Button 4" callback}
	    }
	}
    }
    2x1 {
	{BUTTON "Button 5" callback}
	{BUTTON "Button 6" callback}
    }
}
