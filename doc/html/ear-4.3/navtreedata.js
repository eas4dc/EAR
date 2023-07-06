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
        [ "Other application types or frameworks", "d6/d86/md_User_guide.html#autotoc_md9", null ]
      ] ],
      [ "Retrieving EAR data", "d6/d86/md_User_guide.html#autotoc_md10", null ],
      [ "EAR job submission flags", "d6/d86/md_User_guide.html#ear-job-submission-flags", [
        [ "CPU frequency selection", "d6/d86/md_User_guide.html#autotoc_md11", null ],
        [ "GPU frequency selection", "d6/d86/md_User_guide.html#autotoc_md12", null ]
      ] ],
      [ "Examples", "d6/d86/md_User_guide.html#autotoc_md13", [
        [ "srun examples", "d6/d86/md_User_guide.html#autotoc_md14", null ],
        [ "sbatch + EARL + srun", "d6/d86/md_User_guide.html#autotoc_md15", null ],
        [ "EARL + mpirun", "d6/d86/md_User_guide.html#autotoc_md16", [
          [ "Intel MPI", "d6/d86/md_User_guide.html#autotoc_md17", null ],
          [ "OpenMPI", "d6/d86/md_User_guide.html#openmpi-1", null ]
        ] ]
      ] ],
      [ "EAR job Accounting (eacct)", "d6/d86/md_User_guide.html#eacct-1", [
        [ "Usage examples", "d6/d86/md_User_guide.html#autotoc_md18", null ]
      ] ],
      [ "Job energy optimization: EARL policies", "d6/d86/md_User_guide.html#autotoc_md19", null ]
    ] ],
    [ "EAR commands", "dc/d09/md_EAR_commands.html", [
      [ "EAR job Accounting (eacct)", "dc/d09/md_EAR_commands.html#eacct", null ],
      [ "EAR system energy Report (ereport)", "dc/d09/md_EAR_commands.html#energy-report-ereport", [
        [ "Examples", "dc/d09/md_EAR_commands.html#autotoc_md21", null ],
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
      [ "Loading EAR Library", "d7/d5f/md_EAR_environment_variables.html#autotoc_md23", [
        [ "EAR_LOADER_APPLICATION", "d7/d5f/md_EAR_environment_variables.html#ear_loader_application", null ],
        [ "EAR_LOAD_MPI_VERSION", "d7/d5f/md_EAR_environment_variables.html#ear_load_mpi_version", null ]
      ] ],
      [ "Report plug-ins", "d7/d5f/md_EAR_environment_variables.html#autotoc_md24", [
        [ "EAR_REPORT_ADD", "d7/d5f/md_EAR_environment_variables.html#ear-report-add", null ]
      ] ],
      [ "Verbosity", "d7/d5f/md_EAR_environment_variables.html#verbosity", [
        [ "EARL_VERBOSE_PATH", "d7/d5f/md_EAR_environment_variables.html#earl_verbose_path", null ]
      ] ],
      [ "Frequency management", "d7/d5f/md_EAR_environment_variables.html#autotoc_md25", [
        [ "EAR_GPU_DEF_FREQ", "d7/d5f/md_EAR_environment_variables.html#autotoc_md26", null ],
        [ "EAR_JOB_EXCLUSIVE_MODE", "d7/d5f/md_EAR_environment_variables.html#autotoc_md27", null ],
        [ "Controlling Uncore/Infinity Fabric frequency", "d7/d5f/md_EAR_environment_variables.html#controlling-uncore-infinity-fabric-frequency", [
          [ "EAR_SET_IMCFREQ", "d7/d5f/md_EAR_environment_variables.html#ear_set_imcfreq", null ],
          [ "EAR_MAX_IMCFREQ and EAR_MIN_IMCFREQ", "d7/d5f/md_EAR_environment_variables.html#ear_max_imcfreq-and-ear_min_imcfreq", null ]
        ] ],
        [ "Load Balancing", "d7/d5f/md_EAR_environment_variables.html#load-balancing", null ],
        [ "Support for Intel(R) Speed Select Technology", "d7/d5f/md_EAR_environment_variables.html#support-for-intel-r-speed-select-technology", [
          [ "EAR_PRIO_TASKS", "d7/d5f/md_EAR_environment_variables.html#ear_prio_tasks", null ],
          [ "EAR_PRIO_CPUS", "d7/d5f/md_EAR_environment_variables.html#ear_prio_cpus", null ]
        ] ],
        [ "Disabling EAR's affinity masks usage", "d7/d5f/md_EAR_environment_variables.html#autotoc_md28", null ]
      ] ],
      [ "Data gathering", "d7/d5f/md_EAR_environment_variables.html#autotoc_md29", [
        [ "EAR_GET_MPI_STATS", "d7/d5f/md_EAR_environment_variables.html#ear_get_mpi_stats", null ],
        [ "EAR_TRACE_PLUGIN", "d7/d5f/md_EAR_environment_variables.html#ear_trace_plugin", null ],
        [ "EAR_TRACE_PATH", "d7/d5f/md_EAR_environment_variables.html#autotoc_md30", null ],
        [ "REPORT_EARL_EVENTS", "d7/d5f/md_EAR_environment_variables.html#report_earl_events", [
          [ "Event types", "d7/d5f/md_EAR_environment_variables.html#autotoc_md31", null ]
        ] ]
      ] ]
    ] ],
    [ "Admin guide", "d8/d71/md_Admin_guide.html", [
      [ "EAR Components", "d8/d71/md_Admin_guide.html#ear-components", null ],
      [ "Quick Installation Guide", "d8/d71/md_Admin_guide.html#quick-installation-guide", [
        [ "EAR Requirements", "d8/d71/md_Admin_guide.html#autotoc_md33", null ],
        [ "Compiling and installing EAR", "d8/d71/md_Admin_guide.html#autotoc_md34", null ],
        [ "Deployment and validation", "d8/d71/md_Admin_guide.html#autotoc_md35", [
          [ "Monitoring: Compute node and DB", "d8/d71/md_Admin_guide.html#autotoc_md36", null ],
          [ "Monitoring: EAR plugin", "d8/d71/md_Admin_guide.html#autotoc_md37", null ]
        ] ],
        [ "EAR Library versions: MPI vs. Non-MPI", "d8/d71/md_Admin_guide.html#autotoc_md38", null ]
      ] ],
      [ "Installing from RPM", "d8/d71/md_Admin_guide.html#installing-from-rpm", [
        [ "Installation content", "d8/d71/md_Admin_guide.html#installation-content", null ],
        [ "RPM requirements", "d8/d71/md_Admin_guide.html#rpm-requirements", null ]
      ] ],
      [ "Starting Services", "d8/d71/md_Admin_guide.html#autotoc_md39", null ],
      [ "Updating EAR with a new installation", "d8/d71/md_Admin_guide.html#autotoc_md40", null ],
      [ "Next steps", "d8/d71/md_Admin_guide.html#autotoc_md41", null ]
    ] ],
    [ "Installation from source", "dc/d3a/md_Installation_from_source.html", [
      [ "Requirements", "dc/d3a/md_Installation_from_source.html#autotoc_md43", null ],
      [ "Compilation and installation guide summary", "dc/d3a/md_Installation_from_source.html#autotoc_md44", null ],
      [ "Configure options", "dc/d3a/md_Installation_from_source.html#autotoc_md45", null ],
      [ "Pre-installation fast tweaks", "dc/d3a/md_Installation_from_source.html#autotoc_md46", null ],
      [ "Library distributions/versions", "dc/d3a/md_Installation_from_source.html#autotoc_md47", null ],
      [ "Other useful flags", "dc/d3a/md_Installation_from_source.html#autotoc_md48", null ],
      [ "Installation content", "dc/d3a/md_Installation_from_source.html#autotoc_md49", null ],
      [ "Fine grain tuning of EAR options", "dc/d3a/md_Installation_from_source.html#autotoc_md50", null ],
      [ "Next step", "dc/d3a/md_Installation_from_source.html#autotoc_md51", null ]
    ] ],
    [ "Architecture", "df/dde/md_Architecture.html", [
      [ "EAR Node Manager", "df/dde/md_Architecture.html#ear-node-manager", [
        [ "Overview", "df/dde/md_Architecture.html#autotoc_md53", null ],
        [ "Requirements", "df/dde/md_Architecture.html#autotoc_md54", null ],
        [ "Configuration", "df/dde/md_Architecture.html#autotoc_md55", null ],
        [ "Execution", "df/dde/md_Architecture.html#autotoc_md56", null ],
        [ "Reconfiguration", "df/dde/md_Architecture.html#autotoc_md57", null ]
      ] ],
      [ "EAR Database Manager", "df/dde/md_Architecture.html#ear-database-manager", [
        [ "Configuration", "df/dde/md_Architecture.html#autotoc_md58", null ],
        [ "Execution", "df/dde/md_Architecture.html#autotoc_md59", null ]
      ] ],
      [ "EAR Global Manager", "df/dde/md_Architecture.html#ear-global-manager", [
        [ "Power capping", "df/dde/md_Architecture.html#autotoc_md60", null ],
        [ "Configuration", "df/dde/md_Architecture.html#autotoc_md61", null ],
        [ "Execution", "df/dde/md_Architecture.html#autotoc_md62", null ]
      ] ],
      [ "The EAR Library", "df/dde/md_Architecture.html#the-ear-library", [
        [ "Overview", "df/dde/md_Architecture.html#autotoc_md63", null ],
        [ "Configuration", "df/dde/md_Architecture.html#autotoc_md64", null ],
        [ "Usage", "df/dde/md_Architecture.html#autotoc_md65", null ],
        [ "Policies", "df/dde/md_Architecture.html#autotoc_md66", null ],
        [ "EAR API", "df/dde/md_Architecture.html#autotoc_md67", null ]
      ] ],
      [ "EAR Loader", "df/dde/md_Architecture.html#ear-loader", null ],
      [ "EAR SLURM plugin", "df/dde/md_Architecture.html#ear-slurm-plugin", [
        [ "Configuration", "df/dde/md_Architecture.html#autotoc_md68", null ]
      ] ]
    ] ],
    [ "EAR configuration", "d2/d00/md_Configuration.html", [
      [ "Configuration requirements", "d2/d00/md_Configuration.html#autotoc_md70", [
        [ "EAR paths", "d2/d00/md_Configuration.html#autotoc_md71", null ],
        [ "DB creation and DB server", "d2/d00/md_Configuration.html#autotoc_md72", null ],
        [ "EAR SLURM plug-in", "d2/d00/md_Configuration.html#autotoc_md73", null ]
      ] ],
      [ "EAR configuration file", "d2/d00/md_Configuration.html#ear_conf", [
        [ "EARD configuration", "d2/d00/md_Configuration.html#eard-configuration", null ],
        [ "EARDBD configuration", "d2/d00/md_Configuration.html#eardbd-configuration", null ],
        [ "EARL configuration", "d2/d00/md_Configuration.html#earl-configuration", null ],
        [ "EARGM configuration", "d2/d00/md_Configuration.html#eargm-configuration", [
          [ "Database configuration", "d2/d00/md_Configuration.html#autotoc_md74", null ],
          [ "Common configuration", "d2/d00/md_Configuration.html#autotoc_md75", null ],
          [ "EAR Authorized users/groups/accounts", "d2/d00/md_Configuration.html#autotoc_md76", null ],
          [ "Energy tags", "d2/d00/md_Configuration.html#autotoc_md77", null ]
        ] ],
        [ "Tags", "d2/d00/md_Configuration.html#tags", [
          [ "Power policies plug-ins", "d2/d00/md_Configuration.html#autotoc_md78", null ]
        ] ],
        [ "Island description", "d2/d00/md_Configuration.html#island-description", null ]
      ] ],
      [ "SLURM SPANK plug-in configuration file", "d2/d00/md_Configuration.html#slurm-spank-plugin-configuration-file", [
        [ "MySQL/PostgreSQL", "d2/d00/md_Configuration.html#autotoc_md79", null ],
        [ "MSR Safe", "d2/d00/md_Configuration.html#autotoc_md80", null ]
      ] ]
    ] ],
    [ "Learning phase", "d6/deb/md_Learning_phase.html", [
      [ "Tools", "d6/deb/md_Learning_phase.html#autotoc_md82", [
        [ "Examples", "d6/deb/md_Learning_phase.html#autotoc_md83", null ]
      ] ]
    ] ],
    [ "EAR plug-ins", "df/d86/md_EAR_plug_ins.html", [
      [ "Considerations", "df/d86/md_EAR_plug_ins.html#autotoc_md85", null ]
    ] ],
    [ "EAR Powercap", "d5/de2/md_Powercap.html", [
      [ "Node powercap", "d5/de2/md_Powercap.html#autotoc_md87", null ],
      [ "Cluster powercap", "d5/de2/md_Powercap.html#autotoc_md88", [
        [ "Soft cluster powercap", "d5/de2/md_Powercap.html#autotoc_md89", null ],
        [ "Hard cluster powercap", "d5/de2/md_Powercap.html#autotoc_md90", null ]
      ] ],
      [ "Possible powercap values", "d5/de2/md_Powercap.html#autotoc_md91", null ],
      [ "Example configurations", "d5/de2/md_Powercap.html#autotoc_md92", null ],
      [ "Valid configurations", "d5/de2/md_Powercap.html#autotoc_md93", null ]
    ] ],
    [ "Report plugins", "da/df5/md_Report.html", [
      [ "Prometheus report plugin", "da/df5/md_Report.html#prometheus-report-plugin", [
        [ "Requirements", "da/df5/md_Report.html#autotoc_md95", null ],
        [ "Installation", "da/df5/md_Report.html#autotoc_md96", null ],
        [ "Configuration", "da/df5/md_Report.html#autotoc_md97", null ]
      ] ],
      [ "Examon", "da/df5/md_Report.html#examon", [
        [ "Compilation and installation", "da/df5/md_Report.html#autotoc_md98", null ]
      ] ],
      [ "DCDB", "da/df5/md_Report.html#dcdb", [
        [ "Compilation and configuration", "da/df5/md_Report.html#autotoc_md99", null ]
      ] ],
      [ "Sysfs Report Plugin", "da/df5/md_Report.html#sysfs-report-plugin", [
        [ "Namespace Format", "da/df5/md_Report.html#autotoc_md100", null ],
        [ "Metric File Naming Format", "da/df5/md_Report.html#autotoc_md101", null ],
        [ "Metrics reported", "da/df5/md_Report.html#autotoc_md102", null ]
      ] ]
    ] ],
    [ "EAR Database", "d1/d47/md_EAR_Database.html", [
      [ "Tables", "d1/d47/md_EAR_Database.html#tables", [
        [ "Application information", "d1/d47/md_EAR_Database.html#autotoc_md104", null ],
        [ "System monitoring", "d1/d47/md_EAR_Database.html#autotoc_md105", null ],
        [ "Events", "d1/d47/md_EAR_Database.html#autotoc_md106", null ],
        [ "EARGM reports", "d1/d47/md_EAR_Database.html#autotoc_md107", null ],
        [ "Learning phase", "d1/d47/md_EAR_Database.html#autotoc_md108", null ]
      ] ],
      [ "Creation and maintenance", "d1/d47/md_EAR_Database.html#autotoc_md109", null ],
      [ "Database creation and ear.conf", "d1/d47/md_EAR_Database.html#autotoc_md110", null ],
      [ "Information reported and ear.conf", "d1/d47/md_EAR_Database.html#autotoc_md111", null ],
      [ "Updating from previous versions", "d1/d47/md_EAR_Database.html#updating-from-previous-versions", [
        [ "From EAR 4.2 to 4.3", "d1/d47/md_EAR_Database.html#autotoc_md112", null ],
        [ "From EAR 4.1 to 4.2", "d1/d47/md_EAR_Database.html#autotoc_md113", null ],
        [ "From EAR 3.4 to 4.0", "d1/d47/md_EAR_Database.html#autotoc_md114", null ],
        [ "From EAR 3.3 to 3.4", "d1/d47/md_EAR_Database.html#autotoc_md115", null ]
      ] ],
      [ "Database tables description", "d1/d47/md_EAR_Database.html#autotoc_md116", [
        [ "Jobs", "d1/d47/md_EAR_Database.html#autotoc_md117", null ],
        [ "Applications", "d1/d47/md_EAR_Database.html#autotoc_md118", null ],
        [ "Signatures", "d1/d47/md_EAR_Database.html#autotoc_md119", null ],
        [ "Power_signatures", "d1/d47/md_EAR_Database.html#autotoc_md120", null ],
        [ "GPU_signatures", "d1/d47/md_EAR_Database.html#autotoc_md121", null ],
        [ "Loops", "d1/d47/md_EAR_Database.html#autotoc_md122", null ],
        [ "Events", "d1/d47/md_EAR_Database.html#database-tables-events", null ],
        [ "Global_energy", "d1/d47/md_EAR_Database.html#autotoc_md123", null ],
        [ "Periodic_metrics", "d1/d47/md_EAR_Database.html#autotoc_md124", null ],
        [ "Periodic_aggregations", "d1/d47/md_EAR_Database.html#autotoc_md125", null ]
      ] ]
    ] ],
    [ "Supported systems", "db/dc7/md_Architectures_and_schedulers_supported.html", [
      [ "CPU Models", "db/dc7/md_Architectures_and_schedulers_supported.html#autotoc_md127", null ],
      [ "GPU models", "db/dc7/md_Architectures_and_schedulers_supported.html#autotoc_md128", null ],
      [ "Schedulers", "db/dc7/md_Architectures_and_schedulers_supported.html#autotoc_md129", null ]
    ] ],
    [ "Changelog", "d4/d40/md_CHANGELOG.html", [
      [ "EAR 4.3", "d4/d40/md_CHANGELOG.html#autotoc_md131", null ],
      [ "EAR 4.2", "d4/d40/md_CHANGELOG.html#autotoc_md132", null ],
      [ "EAR4.1.1", "d4/d40/md_CHANGELOG.html#autotoc_md133", null ],
      [ "EAR 4.1", "d4/d40/md_CHANGELOG.html#autotoc_md134", null ],
      [ "EAR 4.0", "d4/d40/md_CHANGELOG.html#autotoc_md135", null ],
      [ "EAR 3.4", "d4/d40/md_CHANGELOG.html#autotoc_md136", null ],
      [ "EAR 3.3", "d4/d40/md_CHANGELOG.html#autotoc_md137", null ],
      [ "EAR 3.2", "d4/d40/md_CHANGELOG.html#autotoc_md138", null ]
    ] ],
    [ "FAQs", "d8/d8a/md_FAQs.html", [
      [ "EAR general questions", "d8/d8a/md_FAQs.html#autotoc_md140", null ],
      [ "Using EAR flags with SLURM plug-in", "d8/d8a/md_FAQs.html#autotoc_md141", null ],
      [ "Using additional MPI profiling libraries/tools", "d8/d8a/md_FAQs.html#autotoc_md142", null ],
      [ "Jobs executed without the EAR Library: Basic Job accounting", "d8/d8a/md_FAQs.html#autotoc_md143", null ],
      [ "Troubleshooting", "d8/d8a/md_FAQs.html#autotoc_md144", null ]
    ] ],
    [ "Known issues", "d1/d21/md_Known_issues.html", null ]
  ] ]
];

var NAVTREEINDEX =
[
"d1/d21/md_Known_issues.html"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';