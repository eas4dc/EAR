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
    [ "User guide", "df/d20/md_User-guide.html", [
      [ "Use cases", "df/d20/md_User-guide.html#use-cases", [
        [ "MPI applications", "df/d20/md_User-guide.html#autotoc_md2", [
          [ "Hybrid MPI + (OpenMP, CUDA, MKL) applications", "df/d20/md_User-guide.html#autotoc_md3", null ],
          [ "Python MPI applications", "df/d20/md_User-guide.html#autotoc_md4", null ],
          [ "Running MPI applications on SLURM systems", "df/d20/md_User-guide.html#autotoc_md5", [
            [ "Using Using mpirun/mpiexec command", "df/d20/md_User-guide.html#using-mpirunmpiexec-command", null ]
          ] ]
        ] ],
        [ "Non-MPI applications", "df/d20/md_User-guide.html#autotoc_md6", [
          [ "Python", "df/d20/md_User-guide.html#autotoc_md7", null ],
          [ "OpenMP, CUDA and Intel MKL", "df/d20/md_User-guide.html#autotoc_md8", null ]
        ] ],
        [ "Other application types or frameworks", "df/d20/md_User-guide.html#autotoc_md9", null ],
        [ "Using EAR inside Singularity containers", "df/d20/md_User-guide.html#autotoc_md10", null ]
      ] ],
      [ "Retrieving EAR data", "df/d20/md_User-guide.html#autotoc_md11", null ],
      [ "EAR job submission flags", "df/d20/md_User-guide.html#ear-job-submission-flags", [
        [ "CPU frequency selection", "df/d20/md_User-guide.html#autotoc_md12", null ],
        [ "GPU frequency selection", "df/d20/md_User-guide.html#autotoc_md13", null ]
      ] ],
      [ "Examples", "df/d20/md_User-guide.html#autotoc_md14", [
        [ "srun examples", "df/d20/md_User-guide.html#autotoc_md15", null ],
        [ "sbatch + EARL + srun", "df/d20/md_User-guide.html#autotoc_md16", null ],
        [ "EARL + mpirun", "df/d20/md_User-guide.html#autotoc_md17", [
          [ "Intel MPI", "df/d20/md_User-guide.html#autotoc_md18", null ],
          [ "OpenMPI", "df/d20/md_User-guide.html#openmpi-1", null ]
        ] ]
      ] ],
      [ "EAR job Accounting (eacct)", "df/d20/md_User-guide.html#eacct-1", [
        [ "Usage examples", "df/d20/md_User-guide.html#autotoc_md19", null ]
      ] ],
      [ "Job energy optimization: EARL policies", "df/d20/md_User-guide.html#autotoc_md20", null ]
    ] ],
    [ "EAR commands", "db/d9e/md_EAR-commands.html", [
      [ "EAR job Accounting (eacct)", "db/d9e/md_EAR-commands.html#eacct", null ],
      [ "EAR system energy Report (ereport)", "db/d9e/md_EAR-commands.html#energy-report-ereport", [
        [ "Examples", "db/d9e/md_EAR-commands.html#autotoc_md22", null ],
        [ "EAR Control (econtrol)", "db/d9e/md_EAR-commands.html#energy-control-econtrol", null ]
      ] ],
      [ "Database commands", "db/d9e/md_EAR-commands.html#database-commands", [
        [ "edb_create", "db/d9e/md_EAR-commands.html#edb_create", null ],
        [ "edb_clean_pm", "db/d9e/md_EAR-commands.html#edb_clean_pm", null ],
        [ "edb_clean_apps", "db/d9e/md_EAR-commands.html#edb_clean_apps", null ]
      ] ],
      [ "erun", "db/d9e/md_EAR-commands.html#erun", null ],
      [ "ear-info", "db/d9e/md_EAR-commands.html#ear-info", null ]
    ] ],
    [ "Environment variables", "d6/d45/md_EAR-environment-variables.html", [
      [ "Loading EAR Library", "d6/d45/md_EAR-environment-variables.html#autotoc_md24", [
        [ "EAR_LOADER_APPLICATION", "d6/d45/md_EAR-environment-variables.html#ear_loader_application", null ],
        [ "EAR_LOAD_MPI_VERSION", "d6/d45/md_EAR-environment-variables.html#ear_load_mpi_version", null ]
      ] ],
      [ "Report plug-ins", "d6/d45/md_EAR-environment-variables.html#autotoc_md25", [
        [ "EAR_REPORT_ADD", "d6/d45/md_EAR-environment-variables.html#ear-report-add", null ]
      ] ],
      [ "Verbosity", "d6/d45/md_EAR-environment-variables.html#verbosity", [
        [ "EARL_VERBOSE_PATH", "d6/d45/md_EAR-environment-variables.html#earl_verbose_path", null ]
      ] ],
      [ "Frequency management", "d6/d45/md_EAR-environment-variables.html#autotoc_md26", [
        [ "EAR_GPU_DEF_FREQ", "d6/d45/md_EAR-environment-variables.html#autotoc_md27", null ],
        [ "EAR_JOB_EXCLUSIVE_MODE", "d6/d45/md_EAR-environment-variables.html#autotoc_md28", null ],
        [ "Controlling Uncore/Infinity Fabric frequency", "d6/d45/md_EAR-environment-variables.html#controlling-uncore-infinity-fabric-frequency", [
          [ "EAR_SET_IMCFREQ", "d6/d45/md_EAR-environment-variables.html#ear_set_imcfreq", null ],
          [ "EAR_MAX_IMCFREQ and EAR_MIN_IMCFREQ", "d6/d45/md_EAR-environment-variables.html#ear_max_imcfreq-and-ear_min_imcfreq", null ]
        ] ],
        [ "Load Balancing", "d6/d45/md_EAR-environment-variables.html#load-balancing", [
          [ "EAR_LOAD_BALANCE", "d6/d45/md_EAR-environment-variables.html#autotoc_md29", null ]
        ] ],
        [ "Support for Intel(R) Speed Select Technology", "d6/d45/md_EAR-environment-variables.html#support-for-intel-r-speed-select-technology", [
          [ "EAR_PRIO_TASKS", "d6/d45/md_EAR-environment-variables.html#ear_prio_tasks", null ],
          [ "EAR_PRIO_CPUS", "d6/d45/md_EAR-environment-variables.html#ear_prio_cpus", null ]
        ] ],
        [ "EAR_MIN_CPUFREQ", "d6/d45/md_EAR-environment-variables.html#autotoc_md30", null ],
        [ "Disabling EAR's affinity masks usage", "d6/d45/md_EAR-environment-variables.html#autotoc_md31", null ]
      ] ],
      [ "Data gathering", "d6/d45/md_EAR-environment-variables.html#autotoc_md32", [
        [ "EAR_GET_MPI_STATS", "d6/d45/md_EAR-environment-variables.html#ear_get_mpi_stats", null ],
        [ "EAR_TRACE_PLUGIN", "d6/d45/md_EAR-environment-variables.html#ear_trace_plugin", null ],
        [ "EAR_TRACE_PATH", "d6/d45/md_EAR-environment-variables.html#autotoc_md33", null ],
        [ "REPORT_EARL_EVENTS", "d6/d45/md_EAR-environment-variables.html#report_earl_events", [
          [ "Event types", "d6/d45/md_EAR-environment-variables.html#autotoc_md34", null ]
        ] ]
      ] ]
    ] ],
    [ "Admin guide", "d5/d16/md_Admin-guide.html", [
      [ "EAR Components", "d5/d16/md_Admin-guide.html#ear-components", null ],
      [ "Quick Installation Guide", "d5/d16/md_Admin-guide.html#quick-installation-guide", [
        [ "EAR Requirements", "d5/d16/md_Admin-guide.html#autotoc_md36", null ],
        [ "Compiling and installing EAR", "d5/d16/md_Admin-guide.html#autotoc_md37", null ],
        [ "Deployment and validation", "d5/d16/md_Admin-guide.html#autotoc_md38", [
          [ "Monitoring: Compute node and DB", "d5/d16/md_Admin-guide.html#autotoc_md39", null ],
          [ "Monitoring: EAR plugin", "d5/d16/md_Admin-guide.html#autotoc_md40", null ]
        ] ],
        [ "EAR Library versions: MPI vs. Non-MPI", "d5/d16/md_Admin-guide.html#autotoc_md41", null ]
      ] ],
      [ "Installing from RPM", "d5/d16/md_Admin-guide.html#installing-from-rpm", [
        [ "Installation content", "d5/d16/md_Admin-guide.html#installation-content", null ],
        [ "RPM requirements", "d5/d16/md_Admin-guide.html#rpm-requirements", null ]
      ] ],
      [ "Starting Services", "d5/d16/md_Admin-guide.html#autotoc_md42", null ],
      [ "Updating EAR with a new installation", "d5/d16/md_Admin-guide.html#autotoc_md43", null ],
      [ "Next steps", "d5/d16/md_Admin-guide.html#autotoc_md44", null ]
    ] ],
    [ "Installation from source", "d4/d78/md_Installation-from-source.html", [
      [ "Requirements", "d4/d78/md_Installation-from-source.html#autotoc_md46", null ],
      [ "Compilation and installation guide summary", "d4/d78/md_Installation-from-source.html#autotoc_md47", null ],
      [ "Configure options", "d4/d78/md_Installation-from-source.html#autotoc_md48", null ],
      [ "Pre-installation fast tweaks", "d4/d78/md_Installation-from-source.html#autotoc_md49", null ],
      [ "Library distributions/versions", "d4/d78/md_Installation-from-source.html#autotoc_md50", null ],
      [ "Other useful flags", "d4/d78/md_Installation-from-source.html#autotoc_md51", null ],
      [ "Installation content", "d4/d78/md_Installation-from-source.html#autotoc_md52", null ],
      [ "Fine grain tuning of EAR options", "d4/d78/md_Installation-from-source.html#autotoc_md53", null ],
      [ "Next step", "d4/d78/md_Installation-from-source.html#autotoc_md54", null ]
    ] ],
    [ "Architecture", "df/dde/md_Architecture.html", [
      [ "EAR Node Manager", "df/dde/md_Architecture.html#ear-node-manager", [
        [ "Overview", "df/dde/md_Architecture.html#autotoc_md56", null ],
        [ "Requirements", "df/dde/md_Architecture.html#autotoc_md57", null ],
        [ "Configuration", "df/dde/md_Architecture.html#autotoc_md58", null ],
        [ "Execution", "df/dde/md_Architecture.html#autotoc_md59", null ],
        [ "Reconfiguration", "df/dde/md_Architecture.html#autotoc_md60", null ]
      ] ],
      [ "EAR Database Manager", "df/dde/md_Architecture.html#ear-database-manager", [
        [ "Configuration", "df/dde/md_Architecture.html#autotoc_md61", null ],
        [ "Execution", "df/dde/md_Architecture.html#autotoc_md62", null ]
      ] ],
      [ "EAR Global Manager", "df/dde/md_Architecture.html#ear-global-manager", [
        [ "Power capping", "df/dde/md_Architecture.html#autotoc_md63", null ],
        [ "Configuration", "df/dde/md_Architecture.html#autotoc_md64", null ],
        [ "Execution", "df/dde/md_Architecture.html#autotoc_md65", null ]
      ] ],
      [ "The EAR Library", "df/dde/md_Architecture.html#the-ear-library", [
        [ "Overview", "df/dde/md_Architecture.html#autotoc_md66", null ],
        [ "Configuration", "df/dde/md_Architecture.html#autotoc_md67", null ],
        [ "Usage", "df/dde/md_Architecture.html#autotoc_md68", null ],
        [ "Policies", "df/dde/md_Architecture.html#autotoc_md69", null ],
        [ "EAR API", "df/dde/md_Architecture.html#autotoc_md70", null ]
      ] ],
      [ "EAR Loader", "df/dde/md_Architecture.html#ear-loader", null ],
      [ "EAR SLURM plugin", "df/dde/md_Architecture.html#ear-slurm-plugin", [
        [ "Configuration", "df/dde/md_Architecture.html#autotoc_md71", null ]
      ] ]
    ] ],
    [ "EAR configuration", "d2/d00/md_Configuration.html", [
      [ "Configuration requirements", "d2/d00/md_Configuration.html#autotoc_md73", [
        [ "EAR paths", "d2/d00/md_Configuration.html#autotoc_md74", null ],
        [ "DB creation and DB server", "d2/d00/md_Configuration.html#autotoc_md75", null ],
        [ "EAR SLURM plug-in", "d2/d00/md_Configuration.html#autotoc_md76", null ]
      ] ],
      [ "EAR configuration file", "d2/d00/md_Configuration.html#ear_conf", [
        [ "EARD configuration", "d2/d00/md_Configuration.html#eard-configuration", null ],
        [ "EARDBD configuration", "d2/d00/md_Configuration.html#eardbd-configuration", null ],
        [ "EARL configuration", "d2/d00/md_Configuration.html#earl-configuration", null ],
        [ "EARGM configuration", "d2/d00/md_Configuration.html#eargm-configuration", [
          [ "Database configuration", "d2/d00/md_Configuration.html#autotoc_md77", null ],
          [ "Common configuration", "d2/d00/md_Configuration.html#autotoc_md78", null ],
          [ "EAR Authorized users/groups/accounts", "d2/d00/md_Configuration.html#autotoc_md79", null ],
          [ "Energy tags", "d2/d00/md_Configuration.html#autotoc_md80", null ]
        ] ],
        [ "Tags", "d2/d00/md_Configuration.html#tags", [
          [ "Power policies plug-ins", "d2/d00/md_Configuration.html#autotoc_md81", null ]
        ] ],
        [ "Island description", "d2/d00/md_Configuration.html#island-description", null ]
      ] ],
      [ "SLURM SPANK plug-in configuration file", "d2/d00/md_Configuration.html#slurm-spank-plugin-configuration-file", [
        [ "MySQL/PostgreSQL", "d2/d00/md_Configuration.html#autotoc_md82", null ],
        [ "MSR Safe", "d2/d00/md_Configuration.html#autotoc_md83", null ]
      ] ]
    ] ],
    [ "Learning phase", "dd/ddd/md_Learning-phase.html", [
      [ "Tools", "dd/ddd/md_Learning-phase.html#autotoc_md85", [
        [ "Examples", "dd/ddd/md_Learning-phase.html#autotoc_md86", null ]
      ] ]
    ] ],
    [ "EAR plug-ins", "dc/db1/md_EAR-plug-ins.html", [
      [ "Considerations", "dc/db1/md_EAR-plug-ins.html#autotoc_md88", null ]
    ] ],
    [ "EAR Powercap", "d5/de2/md_Powercap.html", [
      [ "Node powercap", "d5/de2/md_Powercap.html#autotoc_md90", null ],
      [ "Cluster powercap", "d5/de2/md_Powercap.html#autotoc_md91", [
        [ "Soft cluster powercap", "d5/de2/md_Powercap.html#autotoc_md92", null ],
        [ "Hard cluster powercap", "d5/de2/md_Powercap.html#autotoc_md93", null ]
      ] ],
      [ "Possible powercap values", "d5/de2/md_Powercap.html#autotoc_md94", null ],
      [ "Example configurations", "d5/de2/md_Powercap.html#autotoc_md95", null ],
      [ "Valid configurations", "d5/de2/md_Powercap.html#autotoc_md96", null ]
    ] ],
    [ "Report plugins", "da/df5/md_Report.html", [
      [ "Prometheus report plugin", "da/df5/md_Report.html#prometheus-report-plugin", [
        [ "Requirements", "da/df5/md_Report.html#autotoc_md98", null ],
        [ "Installation", "da/df5/md_Report.html#autotoc_md99", null ],
        [ "Configuration", "da/df5/md_Report.html#autotoc_md100", null ]
      ] ],
      [ "Examon", "da/df5/md_Report.html#examon", [
        [ "Compilation and installation", "da/df5/md_Report.html#autotoc_md101", null ]
      ] ],
      [ "DCDB", "da/df5/md_Report.html#dcdb", [
        [ "Compilation and configuration", "da/df5/md_Report.html#autotoc_md102", null ]
      ] ],
      [ "Sysfs Report Plugin", "da/df5/md_Report.html#sysfs-report-plugin", [
        [ "Namespace Format", "da/df5/md_Report.html#autotoc_md103", null ],
        [ "Metric File Naming Format", "da/df5/md_Report.html#autotoc_md104", null ],
        [ "Metrics reported", "da/df5/md_Report.html#autotoc_md105", null ]
      ] ]
    ] ],
    [ "EAR Database", "d7/d4d/md_EAR-Database.html", [
      [ "Tables", "d7/d4d/md_EAR-Database.html#tables", [
        [ "Application information", "d7/d4d/md_EAR-Database.html#autotoc_md107", null ],
        [ "System monitoring", "d7/d4d/md_EAR-Database.html#autotoc_md108", null ],
        [ "Events", "d7/d4d/md_EAR-Database.html#autotoc_md109", null ],
        [ "EARGM reports", "d7/d4d/md_EAR-Database.html#autotoc_md110", null ],
        [ "Learning phase", "d7/d4d/md_EAR-Database.html#autotoc_md111", null ]
      ] ],
      [ "Creation and maintenance", "d7/d4d/md_EAR-Database.html#autotoc_md112", null ],
      [ "Database creation and ear.conf", "d7/d4d/md_EAR-Database.html#autotoc_md113", null ],
      [ "Information reported and ear.conf", "d7/d4d/md_EAR-Database.html#autotoc_md114", null ],
      [ "Updating from previous versions", "d7/d4d/md_EAR-Database.html#updating-from-previous-versions", [
        [ "From EAR 4.2 to 4.3", "d7/d4d/md_EAR-Database.html#autotoc_md115", null ],
        [ "From EAR 4.1 to 4.2", "d7/d4d/md_EAR-Database.html#autotoc_md116", null ],
        [ "From EAR 3.4 to 4.0", "d7/d4d/md_EAR-Database.html#autotoc_md117", null ],
        [ "From EAR 3.3 to 3.4", "d7/d4d/md_EAR-Database.html#autotoc_md118", null ]
      ] ],
      [ "Database tables description", "d7/d4d/md_EAR-Database.html#autotoc_md119", [
        [ "Jobs", "d7/d4d/md_EAR-Database.html#autotoc_md120", null ],
        [ "Applications", "d7/d4d/md_EAR-Database.html#autotoc_md121", null ],
        [ "Signatures", "d7/d4d/md_EAR-Database.html#autotoc_md122", null ],
        [ "Power_signatures", "d7/d4d/md_EAR-Database.html#autotoc_md123", null ],
        [ "GPU_signatures", "d7/d4d/md_EAR-Database.html#autotoc_md124", null ],
        [ "Loops", "d7/d4d/md_EAR-Database.html#autotoc_md125", null ],
        [ "Events", "d7/d4d/md_EAR-Database.html#database-tables-events", null ],
        [ "Global_energy", "d7/d4d/md_EAR-Database.html#autotoc_md126", null ],
        [ "Periodic_metrics", "d7/d4d/md_EAR-Database.html#autotoc_md127", null ],
        [ "Periodic_aggregations", "d7/d4d/md_EAR-Database.html#autotoc_md128", null ]
      ] ]
    ] ],
    [ "Supported systems", "dc/d4d/md_Architectures-and-schedulers-supported.html", [
      [ "CPU Models", "dc/d4d/md_Architectures-and-schedulers-supported.html#autotoc_md130", null ],
      [ "GPU models", "dc/d4d/md_Architectures-and-schedulers-supported.html#autotoc_md131", null ],
      [ "Schedulers", "dc/d4d/md_Architectures-and-schedulers-supported.html#autotoc_md132", null ]
    ] ],
    [ "Changelog", "d4/d40/md_CHANGELOG.html", [
      [ "EAR 4.3", "d4/d40/md_CHANGELOG.html#autotoc_md134", null ],
      [ "EAR 4.2", "d4/d40/md_CHANGELOG.html#autotoc_md135", null ],
      [ "EAR4.1.1", "d4/d40/md_CHANGELOG.html#autotoc_md136", null ],
      [ "EAR 4.1", "d4/d40/md_CHANGELOG.html#autotoc_md137", null ],
      [ "EAR 4.0", "d4/d40/md_CHANGELOG.html#autotoc_md138", null ],
      [ "EAR 3.4", "d4/d40/md_CHANGELOG.html#autotoc_md139", null ],
      [ "EAR 3.3", "d4/d40/md_CHANGELOG.html#autotoc_md140", null ],
      [ "EAR 3.2", "d4/d40/md_CHANGELOG.html#autotoc_md141", null ]
    ] ],
    [ "FAQs", "d8/d8a/md_FAQs.html", [
      [ "EAR general questions", "d8/d8a/md_FAQs.html#autotoc_md143", null ],
      [ "Using EAR flags with SLURM plug-in", "d8/d8a/md_FAQs.html#autotoc_md144", null ],
      [ "Using additional MPI profiling libraries/tools", "d8/d8a/md_FAQs.html#autotoc_md145", null ],
      [ "Jobs executed without the EAR Library: Basic Job accounting", "d8/d8a/md_FAQs.html#autotoc_md146", null ],
      [ "Troubleshooting", "d8/d8a/md_FAQs.html#autotoc_md147", null ]
    ] ],
    [ "Known issues", "db/df1/md_Known-issues.html", null ]
  ] ]
];

var NAVTREEINDEX =
[
"d2/d00/md_Configuration.html"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';