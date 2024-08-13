/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "EAR", "index.html", [
    [ "Introduction", "index.html", "index" ],
    [ "User guide", "d6/d86/md_User_guide.html", [
      [ "Use cases", "d6/d86/md_User_guide.html#use-cases", [
        [ "MPI applications", "d6/d86/md_User_guide.html#autotoc_md2", [
          [ "Hybrid MPI + (OpenMP, CUDA, MKL) applications", "d6/d86/md_User_guide.html#autotoc_md3", null ],
          [ "Python MPI applications", "d6/d86/md_User_guide.html#autotoc_md4", null ],
          [ "Running MPI applications on SLURM systems", "d6/d86/md_User_guide.html#autotoc_md5", [
            [ "Using Using mpirun/mpiexec command", "d6/d86/md_User_guide.html#using-mpirunmpiexec-command", null ]
          ] ]
        ] ],
        [ "Non-MPI applications", "d6/d86/md_User_guide.html#autotoc_md6", [
          [ "Python", "d6/d86/md_User_guide.html#autotoc_md7", null ],
          [ "OpenMP, CUDA and Intel MKL", "d6/d86/md_User_guide.html#autotoc_md8", null ]
        ] ],
        [ "Other application types or frameworks", "d6/d86/md_User_guide.html#autotoc_md9", null ],
        [ "Using EAR inside Singularity containers", "d6/d86/md_User_guide.html#autotoc_md10", null ]
      ] ],
      [ "Retrieving EAR data", "d6/d86/md_User_guide.html#autotoc_md11", null ],
      [ "EAR job submission flags", "d6/d86/md_User_guide.html#ear-job-submission-flags", [
        [ "CPU frequency selection", "d6/d86/md_User_guide.html#autotoc_md12", null ],
        [ "GPU frequency selection", "d6/d86/md_User_guide.html#autotoc_md13", null ]
      ] ],
      [ "Examples", "d6/d86/md_User_guide.html#autotoc_md14", [
        [ "srun examples", "d6/d86/md_User_guide.html#autotoc_md15", null ],
        [ "sbatch + EARL + srun", "d6/d86/md_User_guide.html#autotoc_md16", null ],
        [ "EARL + mpirun", "d6/d86/md_User_guide.html#autotoc_md17", [
          [ "Intel MPI", "d6/d86/md_User_guide.html#autotoc_md18", null ],
          [ "OpenMPI", "d6/d86/md_User_guide.html#openmpi-1", null ]
        ] ]
      ] ],
      [ "EAR job Accounting (eacct)", "d6/d86/md_User_guide.html#eacct-1", [
        [ "Usage examples", "d6/d86/md_User_guide.html#autotoc_md19", null ]
      ] ],
      [ "Job energy optimization: EARL policies", "d6/d86/md_User_guide.html#autotoc_md20", null ],
      [ "Data visualization with Grafana", "d6/d86/md_User_guide.html#autotoc_md21", null ]
    ] ],
    [ "EAR commands", "dc/d09/md_EAR_commands.html", [
      [ "EAR job Accounting (eacct)", "dc/d09/md_EAR_commands.html#eacct", null ],
      [ "EAR system energy Report (ereport)", "dc/d09/md_EAR_commands.html#energy-report-ereport", [
        [ "Examples", "dc/d09/md_EAR_commands.html#autotoc_md23", null ],
        [ "EAR Control (econtrol)", "dc/d09/md_EAR_commands.html#energy-control-econtrol", null ]
      ] ],
      [ "Database commands", "dc/d09/md_EAR_commands.html#database-commands", [
        [ "edb_create", "dc/d09/md_EAR_commands.html#edb_create", null ],
        [ "edb_clean_pm", "dc/d09/md_EAR_commands.html#edb_clean_pm", null ],
        [ "edb_clean_apps", "dc/d09/md_EAR_commands.html#edb_clean_apps", null ]
      ] ],
      [ "erun", "dc/d09/md_EAR_commands.html#erun", null ],
      [ "ear-info", "dc/d09/md_EAR_commands.html#ear-info", null ]
    ] ],
    [ "Environment variables", "d7/d5f/md_EAR_environment_variables.html", [
      [ "Loading EAR Library", "d7/d5f/md_EAR_environment_variables.html#autotoc_md25", [
        [ "EAR_LOADER_APPLICATION", "d7/d5f/md_EAR_environment_variables.html#ear_loader_application", null ],
        [ "EAR_LOAD_MPI_VERSION", "d7/d5f/md_EAR_environment_variables.html#ear_load_mpi_version", null ]
      ] ],
      [ "Report plug-ins", "d7/d5f/md_EAR_environment_variables.html#autotoc_md26", [
        [ "EAR_REPORT_ADD", "d7/d5f/md_EAR_environment_variables.html#ear-report-add", null ]
      ] ],
      [ "Verbosity", "d7/d5f/md_EAR_environment_variables.html#verbosity", [
        [ "EARL_VERBOSE_PATH", "d7/d5f/md_EAR_environment_variables.html#earl_verbose_path", null ]
      ] ],
      [ "Frequency management", "d7/d5f/md_EAR_environment_variables.html#autotoc_md27", [
        [ "EAR_GPU_DEF_FREQ", "d7/d5f/md_EAR_environment_variables.html#autotoc_md28", null ],
        [ "EAR_JOB_EXCLUSIVE_MODE", "d7/d5f/md_EAR_environment_variables.html#autotoc_md29", null ],
        [ "Controlling Uncore/Infinity Fabric frequency", "d7/d5f/md_EAR_environment_variables.html#controlling-uncore-infinity-fabric-frequency", [
          [ "EAR_SET_IMCFREQ", "d7/d5f/md_EAR_environment_variables.html#ear_set_imcfreq", null ],
          [ "EAR_MAX_IMCFREQ and EAR_MIN_IMCFREQ", "d7/d5f/md_EAR_environment_variables.html#ear_max_imcfreq-and-ear_min_imcfreq", null ]
        ] ],
        [ "Load Balancing", "d7/d5f/md_EAR_environment_variables.html#load-balancing", [
          [ "EAR_LOAD_BALANCE", "d7/d5f/md_EAR_environment_variables.html#autotoc_md30", null ]
        ] ],
        [ "Support for Intel(R) Speed Select Technology", "d7/d5f/md_EAR_environment_variables.html#support-for-intel-r-speed-select-technology", [
          [ "EAR_PRIO_TASKS", "d7/d5f/md_EAR_environment_variables.html#ear_prio_tasks", null ],
          [ "EAR_PRIO_CPUS", "d7/d5f/md_EAR_environment_variables.html#ear_prio_cpus", null ]
        ] ],
        [ "EAR_MIN_CPUFREQ", "d7/d5f/md_EAR_environment_variables.html#autotoc_md31", null ],
        [ "Disabling EAR's affinity masks usage", "d7/d5f/md_EAR_environment_variables.html#autotoc_md32", null ],
        [ "Workflow support", "d7/d5f/md_EAR_environment_variables.html#autotoc_md33", [
          [ "EAR_DISABLE_NODE_METRICS", "d7/d5f/md_EAR_environment_variables.html#autotoc_md34", null ],
          [ "EAR_NTASK_WORK_SHARING", "d7/d5f/md_EAR_environment_variables.html#autotoc_md35", null ]
        ] ],
        [ "Data gathering/reporting", "d7/d5f/md_EAR_environment_variables.html#autotoc_md36", null ],
        [ "EARL_REPORT_LOOPS", "d7/d5f/md_EAR_environment_variables.html#autotoc_md37", null ],
        [ "EAR_GET_MPI_STATS", "d7/d5f/md_EAR_environment_variables.html#ear_get_mpi_stats", null ],
        [ "EAR_TRACE_PLUGIN", "d7/d5f/md_EAR_environment_variables.html#ear_trace_plugin", null ],
        [ "EAR_TRACE_PATH", "d7/d5f/md_EAR_environment_variables.html#autotoc_md38", null ],
        [ "REPORT_EARL_EVENTS", "d7/d5f/md_EAR_environment_variables.html#report_earl_events", [
          [ "Event types", "d7/d5f/md_EAR_environment_variables.html#autotoc_md39", null ]
        ] ]
      ] ]
    ] ],
    [ "Admin guide", "d8/d71/md_Admin_guide.html", [
      [ "EAR Components", "d8/d71/md_Admin_guide.html#ear-components", null ],
      [ "Quick Installation Guide", "d8/d71/md_Admin_guide.html#quick-installation-guide", [
        [ "EAR Requirements", "d8/d71/md_Admin_guide.html#autotoc_md41", null ],
        [ "Compiling and installing EAR", "d8/d71/md_Admin_guide.html#autotoc_md42", null ],
        [ "Deployment and validation", "d8/d71/md_Admin_guide.html#autotoc_md43", [
          [ "Monitoring: Compute node and DB", "d8/d71/md_Admin_guide.html#autotoc_md44", null ],
          [ "Monitoring: EAR plugin", "d8/d71/md_Admin_guide.html#autotoc_md45", null ]
        ] ],
        [ "EAR Library versions: MPI vs. Non-MPI", "d8/d71/md_Admin_guide.html#autotoc_md46", null ]
      ] ],
      [ "Installing from RPM", "d8/d71/md_Admin_guide.html#installing-from-rpm", [
        [ "Installation content", "d8/d71/md_Admin_guide.html#installation-content", null ],
        [ "RPM requirements", "d8/d71/md_Admin_guide.html#rpm-requirements", null ]
      ] ],
      [ "Starting Services", "d8/d71/md_Admin_guide.html#autotoc_md47", null ],
      [ "Updating EAR with a new installation", "d8/d71/md_Admin_guide.html#autotoc_md48", null ],
      [ "Next steps", "d8/d71/md_Admin_guide.html#autotoc_md49", null ]
    ] ],
    [ "Installation from source", "dc/d3a/md_Installation_from_source.html", [
      [ "Requirements", "dc/d3a/md_Installation_from_source.html#autotoc_md51", null ],
      [ "Compilation and installation guide summary", "dc/d3a/md_Installation_from_source.html#autotoc_md52", null ],
      [ "Configure options", "dc/d3a/md_Installation_from_source.html#autotoc_md53", null ],
      [ "Pre-installation fast tweaks", "dc/d3a/md_Installation_from_source.html#autotoc_md54", null ],
      [ "Library distributions/versions", "dc/d3a/md_Installation_from_source.html#autotoc_md55", null ],
      [ "Other useful flags", "dc/d3a/md_Installation_from_source.html#autotoc_md56", null ],
      [ "Installation content", "dc/d3a/md_Installation_from_source.html#autotoc_md57", null ],
      [ "Fine grain tuning of EAR options", "dc/d3a/md_Installation_from_source.html#autotoc_md58", null ],
      [ "Next step", "dc/d3a/md_Installation_from_source.html#autotoc_md59", null ]
    ] ],
    [ "Architecture", "df/dde/md_Architecture.html", [
      [ "EAR Node Manager", "df/dde/md_Architecture.html#ear-node-manager", [
        [ "Overview", "df/dde/md_Architecture.html#autotoc_md61", null ],
        [ "Requirements", "df/dde/md_Architecture.html#autotoc_md62", null ],
        [ "Configuration", "df/dde/md_Architecture.html#autotoc_md63", null ],
        [ "Execution", "df/dde/md_Architecture.html#autotoc_md64", null ],
        [ "Reconfiguration", "df/dde/md_Architecture.html#autotoc_md65", null ]
      ] ],
      [ "EAR Database Manager", "df/dde/md_Architecture.html#ear-database-manager", [
        [ "Configuration", "df/dde/md_Architecture.html#autotoc_md66", null ],
        [ "Execution", "df/dde/md_Architecture.html#autotoc_md67", null ]
      ] ],
      [ "EAR Global Manager (System power manager)", "df/dde/md_Architecture.html#ear-global-manager", [
        [ "Power capping", "df/dde/md_Architecture.html#autotoc_md68", null ],
        [ "Configuration", "df/dde/md_Architecture.html#autotoc_md69", null ],
        [ "Execution", "df/dde/md_Architecture.html#autotoc_md70", null ]
      ] ],
      [ "The EAR Library (Job Manager)", "df/dde/md_Architecture.html#the-ear-library", [
        [ "Overview", "df/dde/md_Architecture.html#autotoc_md71", null ],
        [ "Configuration", "df/dde/md_Architecture.html#autotoc_md72", null ],
        [ "Usage", "df/dde/md_Architecture.html#autotoc_md73", null ],
        [ "Policies", "df/dde/md_Architecture.html#autotoc_md74", null ]
      ] ],
      [ "EAR Loader", "df/dde/md_Architecture.html#ear-loader", null ],
      [ "EAR SLURM plugin", "df/dde/md_Architecture.html#ear-slurm-plugin", [
        [ "Configuration", "df/dde/md_Architecture.html#autotoc_md75", null ],
        [ "EAR application API", "df/dde/md_Architecture.html#autotoc_md76", null ]
      ] ]
    ] ],
    [ "High availability support", "d1/d0e/md_High_availability.html", [
      [ "EAR Database Manager HA", "d1/d0e/md_High_availability.html#autotoc_md78", null ],
      [ "EAR Database HA", "d1/d0e/md_High_availability.html#autotoc_md79", null ]
    ] ],
    [ "EAR configuration", "d2/d00/md_Configuration.html", [
      [ "EAR Configuration requirements", "d2/d00/md_Configuration.html#autotoc_md81", [
        [ "EAR paths", "d2/d00/md_Configuration.html#autotoc_md82", null ],
        [ "DB creation and DB server", "d2/d00/md_Configuration.html#autotoc_md83", null ],
        [ "EAR SLURM plug-in", "d2/d00/md_Configuration.html#autotoc_md84", null ]
      ] ],
      [ "EAR configuration file", "d2/d00/md_Configuration.html#ear_conf", [
        [ "Database configuration", "d2/d00/md_Configuration.html#autotoc_md85", null ],
        [ "EARD configuration", "d2/d00/md_Configuration.html#eard-configuration", null ],
        [ "EARDBD configuration", "d2/d00/md_Configuration.html#eardbd-configuration", null ],
        [ "EARL configuration", "d2/d00/md_Configuration.html#earl-configuration", null ],
        [ "EARGM configuration", "d2/d00/md_Configuration.html#eargm-configuration", null ],
        [ "Common configuration", "d2/d00/md_Configuration.html#autotoc_md86", null ],
        [ "EAR Authorized users/groups/accounts", "d2/d00/md_Configuration.html#autotoc_md87", null ],
        [ "Energy tags", "d2/d00/md_Configuration.html#autotoc_md88", null ],
        [ "Tags", "d2/d00/md_Configuration.html#tags", null ],
        [ "Power policies plug-ins", "d2/d00/md_Configuration.html#autotoc_md89", null ],
        [ "Island description", "d2/d00/md_Configuration.html#island-description", null ],
        [ "EDCMON", "d2/d00/md_Configuration.html#autotoc_md90", null ]
      ] ],
      [ "SLURM SPANK plug-in configuration file", "d2/d00/md_Configuration.html#slurm-spank-plugin-configuration-file", null ],
      [ "MySQL/PostgreSQL", "d2/d00/md_Configuration.html#autotoc_md91", null ],
      [ "MSR Safe", "d2/d00/md_Configuration.html#autotoc_md92", null ]
    ] ],
    [ "Learning phase", "d6/deb/md_Learning_phase.html", [
      [ "Tools", "d6/deb/md_Learning_phase.html#autotoc_md94", [
        [ "Examples", "d6/deb/md_Learning_phase.html#autotoc_md95", null ]
      ] ]
    ] ],
    [ "EAR plug-ins", "df/d86/md_EAR_plug_ins.html", [
      [ "Considerations", "df/d86/md_EAR_plug_ins.html#autotoc_md97", null ]
    ] ],
    [ "EAR Powercap", "d5/de2/md_Powercap.html", [
      [ "Node powercap", "d5/de2/md_Powercap.html#autotoc_md99", null ],
      [ "Cluster powercap", "d5/de2/md_Powercap.html#autotoc_md100", [
        [ "Soft cluster powercap", "d5/de2/md_Powercap.html#autotoc_md101", null ],
        [ "Hard cluster powercap", "d5/de2/md_Powercap.html#autotoc_md102", null ]
      ] ],
      [ "Possible powercap values", "d5/de2/md_Powercap.html#autotoc_md103", null ],
      [ "Example configurations", "d5/de2/md_Powercap.html#autotoc_md104", null ],
      [ "Valid configurations", "d5/de2/md_Powercap.html#autotoc_md105", null ]
    ] ],
    [ "Report plugins", "da/df5/md_Report.html", [
      [ "Prometheus report plugin", "da/df5/md_Report.html#prometheus-report-plugin", [
        [ "Requirements", "da/df5/md_Report.html#autotoc_md107", null ],
        [ "Installation", "da/df5/md_Report.html#autotoc_md108", null ],
        [ "Configuration", "da/df5/md_Report.html#autotoc_md109", null ]
      ] ],
      [ "Examon", "da/df5/md_Report.html#examon", [
        [ "Compilation and installation", "da/df5/md_Report.html#autotoc_md110", null ]
      ] ],
      [ "DCDB", "da/df5/md_Report.html#dcdb", [
        [ "Compilation and configuration", "da/df5/md_Report.html#autotoc_md111", null ]
      ] ],
      [ "Sysfs Report Plugin", "da/df5/md_Report.html#sysfs-report-plugin", [
        [ "Namespace Format", "da/df5/md_Report.html#autotoc_md112", null ],
        [ "Metric File Naming Format", "da/df5/md_Report.html#autotoc_md113", null ],
        [ "Metrics reported", "da/df5/md_Report.html#autotoc_md114", null ]
      ] ]
    ] ],
    [ "EAR Database", "d1/d47/md_EAR_Database.html", [
      [ "Tables", "d1/d47/md_EAR_Database.html#tables", [
        [ "Application information", "d1/d47/md_EAR_Database.html#autotoc_md116", null ],
        [ "System monitoring", "d1/d47/md_EAR_Database.html#autotoc_md117", null ],
        [ "Events", "d1/d47/md_EAR_Database.html#autotoc_md118", null ],
        [ "EARGM reports", "d1/d47/md_EAR_Database.html#autotoc_md119", null ],
        [ "Learning phase", "d1/d47/md_EAR_Database.html#autotoc_md120", null ]
      ] ],
      [ "Creation and maintenance", "d1/d47/md_EAR_Database.html#autotoc_md121", null ],
      [ "Database creation and ear.conf", "d1/d47/md_EAR_Database.html#autotoc_md122", null ],
      [ "Information reported and ear.conf", "d1/d47/md_EAR_Database.html#autotoc_md123", null ],
      [ "Updating from previous versions", "d1/d47/md_EAR_Database.html#updating-from-previous-versions", [
        [ "From EAR 4.3 to 5.0", "d1/d47/md_EAR_Database.html#autotoc_md124", null ],
        [ "From EAR 4.2 to 4.3", "d1/d47/md_EAR_Database.html#autotoc_md125", null ],
        [ "From EAR 4.1 to 4.2", "d1/d47/md_EAR_Database.html#autotoc_md126", null ],
        [ "From EAR 3.4 to 4.0", "d1/d47/md_EAR_Database.html#autotoc_md127", null ],
        [ "From EAR 3.3 to 3.4", "d1/d47/md_EAR_Database.html#autotoc_md128", null ]
      ] ],
      [ "Database tables description", "d1/d47/md_EAR_Database.html#autotoc_md129", [
        [ "Jobs", "d1/d47/md_EAR_Database.html#autotoc_md130", null ],
        [ "Applications", "d1/d47/md_EAR_Database.html#autotoc_md131", null ],
        [ "Signatures", "d1/d47/md_EAR_Database.html#autotoc_md132", null ],
        [ "Power_signatures", "d1/d47/md_EAR_Database.html#autotoc_md133", null ],
        [ "GPU_signatures", "d1/d47/md_EAR_Database.html#autotoc_md134", null ],
        [ "Loops", "d1/d47/md_EAR_Database.html#autotoc_md135", null ],
        [ "Events", "d1/d47/md_EAR_Database.html#database-tables-events", null ],
        [ "Global_energy", "d1/d47/md_EAR_Database.html#autotoc_md136", null ],
        [ "Periodic_metrics", "d1/d47/md_EAR_Database.html#autotoc_md137", null ],
        [ "Periodic_aggregations", "d1/d47/md_EAR_Database.html#autotoc_md138", null ]
      ] ]
    ] ],
    [ "Supported systems", "db/dc7/md_Architectures_and_schedulers_supported.html", [
      [ "CPU Models", "db/dc7/md_Architectures_and_schedulers_supported.html#autotoc_md140", null ],
      [ "GPU models", "db/dc7/md_Architectures_and_schedulers_supported.html#autotoc_md141", null ],
      [ "Schedulers", "db/dc7/md_Architectures_and_schedulers_supported.html#autotoc_md142", null ]
    ] ],
    [ "Energy Data Center Monitor", "dd/d67/md_EDCMON.html", [
      [ "The EDCMON executable", "dd/d67/md_EDCMON.html#autotoc_md144", null ],
      [ "EDCMON plugins", "dd/d67/md_EDCMON.html#autotoc_md145", null ],
      [ "Creating new plugins", "dd/d67/md_EDCMON.html#autotoc_md146", [
        [ "Helper macros", "dd/d67/md_EDCMON.html#autotoc_md147", null ]
      ] ],
      [ "Plugin Manager functions", "dd/d67/md_EDCMON.html#autotoc_md148", [
        [ "Other plugins already available", "dd/d67/md_EDCMON.html#autotoc_md149", null ]
      ] ],
      [ "FAQ", "dd/d67/md_EDCMON.html#autotoc_md150", null ]
    ] ],
    [ "Changelog", "d4/d40/md_CHANGELOG.html", [
      [ "EAR 5.0", "d4/d40/md_CHANGELOG.html#autotoc_md152", null ],
      [ "EAR 4.3.1", "d4/d40/md_CHANGELOG.html#autotoc_md153", null ],
      [ "EAR 4.3", "d4/d40/md_CHANGELOG.html#autotoc_md154", null ],
      [ "EAR 4.2", "d4/d40/md_CHANGELOG.html#autotoc_md155", null ],
      [ "EAR 4.1.1", "d4/d40/md_CHANGELOG.html#autotoc_md156", null ],
      [ "EAR 4.1", "d4/d40/md_CHANGELOG.html#autotoc_md157", null ],
      [ "EAR 4.0", "d4/d40/md_CHANGELOG.html#autotoc_md158", null ],
      [ "EAR 3.4", "d4/d40/md_CHANGELOG.html#autotoc_md159", null ],
      [ "EAR 3.3", "d4/d40/md_CHANGELOG.html#autotoc_md160", null ],
      [ "EAR 3.2", "d4/d40/md_CHANGELOG.html#autotoc_md161", null ]
    ] ],
    [ "FAQs", "d8/d8a/md_FAQs.html", [
      [ "EAR general questions", "d8/d8a/md_FAQs.html#autotoc_md163", null ],
      [ "Using EAR flags with SLURM plug-in", "d8/d8a/md_FAQs.html#autotoc_md164", null ],
      [ "Using additional MPI profiling libraries/tools", "d8/d8a/md_FAQs.html#autotoc_md165", null ],
      [ "Jobs executed without the EAR Library: Basic Job accounting", "d8/d8a/md_FAQs.html#autotoc_md166", null ],
      [ "Troubleshooting", "d8/d8a/md_FAQs.html#autotoc_md167", null ]
    ] ],
    [ "Known issues", "d1/d21/md_Known_issues.html", null ]
  ] ]
];

var NAVTREEINDEX =
[
"d1/d0e/md_High_availability.html"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';