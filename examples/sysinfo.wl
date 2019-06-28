uname_tab 1x4 { * 1x1
    {"System Information" bold}
    {VOID 1x1}
    2x4 {
        {> "System:"} sysname {< "?"}
        {> "Release:"} release {< "?"}
        {> "Machine:"} machine {< "?"}
        {> "Hostname:"} hostname {< "?"}
    }
    {VOID 1x2}
}

ram_tab 1x4 { * 1x1
    {"Memory Usage" bold}
    {VOID 1x1}
    2x2 {
        {> "Ram"} ram_usage {PROGRESSBAR 0 1 0}
        {> "Swap"} swap_usage {PROGRESSBAR 0 1 0}
    }
    {VOID 1x2}
}

cpu_tab 1x2 {
    {"CPU Usage" bold}
    {}
}

# Creates three tabs
layout 1x1 {
    "System" "CPU" "Memory" {
        &uname_tab
        &cpu_tab
        &ram_tab
    }
}
