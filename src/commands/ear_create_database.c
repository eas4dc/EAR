/**************************************************************
*   Energy Aware Runtime (EAR)
*   This program is part of the Energy Aware Runtime (EAR).
*
*   EAR provides a dynamic, transparent and ligth-weigth solution for
*   Energy management.
*
*       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*   BSC Contact     mailto:ear-support@bsc.es
*   Lenovo contact  mailto:hpchelp@lenovo.com
*
*   EAR is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Lesser General Public
*   License as published by the Free Software Foundation; either
*   version 2.1 of the License, or (at your option) any later version.
*   
*   EAR is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Lesser General Public License for more details.
*   
*   You should have received a copy of the GNU Lesser General Public
*   License along with EAR; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*   The GNU LEsser General Public License is contained in the file COPYING  
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <mysql/mysql.h>
#include <common/system/user.h>
#include <common/config.h>
#include <common/types/version.h>
#include <common/output/verbose.h>
#include <common/database/db_helper.h>
#include <common/types/configuration/cluster_conf.h>

char print_out = 0;
cluster_conf_t my_cluster;
char signature_detail = !DB_SIMPLE;
char db_node_detail = DEMO;

void usage(char *app)
{
	fprintf(stdout, "Usage:%s [options]\n", app);
    fprintf(stdout, "\t-p\t\tSpecify the password for MySQL's root user.\n");
    fprintf(stdout, "\t-o\t\tOutputs the commands that would run.\n");
    fprintf(stdout, "\t-r\t\tRuns the program. If '-o' this option will be override.\n");
    fprintf(stdout, "\t-v\t\tShows current EAR version.\n");
    fprintf(stdout, "\t-h\t\tShows this message.\n");
	exit(0);
}

#if DB_MYSQL
void execute_on_error(MYSQL *connection)
{
    fprintf(stdout, "Error: %s\n", mysql_error(connection)); //error
    return;
}
#endif

int get_num_indexes(void *connection, char *table)
{
    char query[256];
    int num_indexes;
#if DB_MYSQL
    init_db_helper(&my_cluster.database);
    sprintf(query, "SELECT COUNT(1) IndexIsThere FROM INFORMATION_SCHEMA.STATISTICS WHERE table_schema=DATABASE() AND table_name='%s'", table);
    num_indexes = run_query_int_result(query);
#elif DB_PSQL
    sprintf(query, "SELECT tablename, indexname FROM pg_indexes WHERE tablename = '%s'", table);
    num_indexes = get_num_rows(connection, query);
#endif
    return num_indexes;
}

void run_query(void *connection, char *query)
{
    if (print_out) {
        fprintf(stdout, "%s\n", query);
    } else {
#if DB_MYSQL
        if (mysql_query((MYSQL *)connection, query)) {
            execute_on_error(connection);
        }
#elif DB_PSQL
        PGresult *res = PQexecParams((PGconn *)connection, query, 0, NULL, NULL, NULL, NULL, 1);
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            fprintf(stdout, "%s \n", PQresultErrorMessage(res));
        }
#endif
    }
}

void create_users(void *connection, char *db_name, char *db_user, char *db_user_pass, char *commands_user, char *commands_pass)
{
    char query[256];

#if DB_MYSQL
    sprintf(query, "CREATE USER '%s'@'%%'", db_user);
#elif DB_PSQL
    sprintf(query, "CREATE USER %s", db_user);
#endif
    run_query(connection, query);
    if (strlen(db_user_pass) > 1)
    {
#if DB_MYSQL
        sprintf(query, "SET PASSWORD FOR '%s'@'%%' = PASSWORD('%s')", db_user, db_user_pass);
#elif DB_PSQL
        sprintf(query, "ALTER USER %s WITH ENCRYPTED PASSWORD '%s'", db_user, db_user_pass);
#endif
        run_query(connection, query);
    }

#if DB_MYSQL
    sprintf(query, "GRANT SELECT, INSERT ON %s.* TO '%s'@'%%'", db_name, db_user);
#elif DB_PSQL
    sprintf(query, "GRANT SELECT, UPDATE ON ALL SEQUENCES IN SCHEMA public TO %s", db_user);
    run_query(connection, query);
    sprintf(query, "GRANT SELECT, INSERT ON ALL TABLES IN SCHEMA public TO %s", db_user);
#endif
    run_query(connection, query);
    
    if (strlen(commands_user))
    {       
#if DB_MYSQL
        sprintf(query, "CREATE USER '%s'@'%%'", commands_user);
#elif DB_PSQL
        sprintf(query, "CREATE USER %s", commands_user);
#endif
        run_query(connection, query);

        if (strlen(commands_pass) > 1)
        {
#if DB_MYSQL
            sprintf(query, "SET PASSWORD FOR '%s'@'%%' = PASSWORD('%s')", commands_user, commands_pass);
#elif DB_PSQL
            sprintf(query, "ALTER USER %s WITH ENCRYPTED PASSWORD '%s'", commands_user, commands_pass);
#endif
            run_query(connection, query);
        }
    
#if DB_MYSQL
        sprintf(query, "GRANT SELECT ON %s.* TO '%s'@'%%'", db_name, commands_user);
#elif DB_PSQL
        sprintf(query, "GRANT SELECT ON ALL TABLES IN SCHEMA public TO %s", commands_user);
#endif
        run_query(connection, query);
    
#if DB_MYSQL
        sprintf(query, "ALTER USER '%s'@'%%' WITH MAX_USER_CONNECTIONS %d", commands_user, my_cluster.database.max_connections);
#elif DB_PSQL
        sprintf(query, "ALTER USER %s WITH CONNECTION LIMIT %d", commands_user, my_cluster.database.max_connections);
#endif
        run_query(connection, query);
    }
#if DB_MYSQL
    sprintf(query, "FLUSH PRIVILEGES");
    run_query(connection, query);
#endif
}

void create_db(void *connection, char *db_name)
{
    char query[256];

#if DB_MYSQL
    sprintf(query, "CREATE DATABASE IF NOT EXISTS %s", db_name);
#elif DB_PSQL
    sprintf(query, "CREATE DATABASE %s", db_name);
#endif
    run_query(connection, query);

#if DB_MYSQL
    sprintf(query, "USE %s", db_name);
    run_query(connection, query);
#endif
}

void create_indexes(void *connection)
{
    char query[256];
    if (get_num_indexes(connection, "Periodic_metrics") < 3)
    {
        sprintf(query, "CREATE INDEX idx_end_time ON Periodic_metrics (end_time)");
        run_query(connection, query);

        sprintf(query, "CREATE INDEX idx_job_id ON Periodic_metrics (job_id)");
        run_query(connection, query);
    } else {
        fprintf(stdout, "Periodic_metrics indexes already created, skipping...\n");
    }

    if (get_num_indexes(connection, "Jobs") < 3)
    {
        sprintf(query, "CREATE INDEX idx_user_id ON Jobs (user_id)");
        run_query(connection, query);
    }
    else {
        fprintf(stdout, "Jobs indexes already created, skipping...\n");
    }
    
}

#if DB_MYSQL
void create_tables(void *connection)
{
    char query[1024];
    sprintf(query, "CREATE TABLE IF NOT EXISTS Applications (\
job_id INT unsigned NOT NULL, \
step_id INT unsigned NOT NULL, \
node_id VARCHAR(128), \
signature_id INT unsigned, \
power_signature_id INT unsigned, \
PRIMARY KEY(job_id, step_id, node_id))");
    run_query(connection, query);

    sprintf(query, "CREATE TABLE IF NOT EXISTS Loops ( \
event INT unsigned NOT NULL, \
size INT unsigned NOT NULL, \
level INT unsigned NOT NULL, \
job_id INT unsigned NOT NULL, \
step_id INT unsigned NOT NULL, \
node_id VARCHAR(8), \
total_iterations INT unsigned, \
signature_id INT unsigned)");
    run_query(connection, query);

    sprintf(query, "CREATE TABLE IF NOT EXISTS Jobs (\
id INT unsigned NOT NULL,\
step_id INT unsigned NOT NULL, \
user_id VARCHAR(128),\
app_id VARCHAR(128),\
start_time INT NOT NULL,\
end_time INT NOT NULL,\
start_mpi_time INT NOT NULL,\
end_mpi_time INT NOT NULL,\
policy VARCHAR(256) NOT NULL,\
threshold FLOAT NOT NULL,\
procs INT unsigned NOT NULL,\
job_type SMALLINT unsigned NOT NULL,\
def_f INT unsigned, \
user_acc VARCHAR(256), \
user_group VARCHAR(256), \
e_tag VARCHAR(256), \
PRIMARY KEY(id, step_id))");
    run_query(connection, query);

    if (signature_detail)
        sprintf(query, "CREATE TABLE IF NOT EXISTS Signatures (\
id INT unsigned NOT NULL AUTO_INCREMENT,\
DC_power FLOAT,\
DRAM_power FLOAT,\
PCK_power FLOAT,\
GPU_power FLOAT,\
EDP FLOAT,\
GBS FLOAT,\
TPI FLOAT,\
CPI FLOAT,\
Gflops FLOAT,\
time FLOAT,\
FLOPS1 INT unsigned,\
FLOPS2 INT unsigned,\
FLOPS3 INT unsigned,\
FLOPS4 INT unsigned,\
FLOPS5 INT unsigned,\
FLOPS6 INT unsigned,\
FLOPS7 INT unsigned,\
FLOPS8 INT unsigned,\
instructions INT unsigned, \
cycles INT unsigned,\
avg_f INT unsigned,\
def_f INT unsigned, \
PRIMARY KEY (id))");
    else
        sprintf(query, "CREATE TABLE IF NOT EXISTS Signatures (\
id INT unsigned NOT NULL AUTO_INCREMENT,\
DC_power FLOAT,\
DRAM_power FLOAT,\
PCK_power FLOAT,\
GPU_power FLOAT,\
EDP FLOAT,\
GBS FLOAT,\
TPI FLOAT,\
CPI FLOAT,\
Gflops FLOAT,\
time FLOAT,\
avg_f INT unsigned,\
def_f INT unsigned, \
PRIMARY KEY (id))");
    run_query(connection, query);

if (db_node_detail){
    sprintf(query, "CREATE TABLE IF NOT EXISTS Periodic_metrics ( \
id INT unsigned NOT NULL AUTO_INCREMENT, \
start_time INT NOT NULL, \
end_time INT NOT NULL, \
DC_energy INT unsigned NOT NULL, \
node_id VARCHAR(256) NOT NULL, \
job_id INT unsigned NOT NULL, \
step_id INT unsigned NOT NULL, \
avg_f INT, \
temp INT, \
DRAM_energy INT, \
PCK_energy INT, \
GPU_energy INT, \
PRIMARY KEY (id))");
}else{
    sprintf(query, "CREATE TABLE IF NOT EXISTS Periodic_metrics ( \
id INT unsigned NOT NULL AUTO_INCREMENT, \
start_time INT NOT NULL, \
end_time INT NOT NULL, \
DC_energy INT unsigned NOT NULL, \
node_id VARCHAR(256) NOT NULL, \
job_id INT unsigned NOT NULL, \
step_id INT unsigned NOT NULL, "
"PRIMARY KEY (id))");
}
    run_query(connection, query);

    sprintf(query, "CREATE TABLE IF NOT EXISTS Power_signatures (  \
id INT unsigned NOT NULL AUTO_INCREMENT, \
DC_power FLOAT NOT NULL, \
DRAM_power FLOAT NOT NULL, \
PCK_power FLOAT NOT NULL, \
EDP FLOAT NOT NULL, \
max_DC_power FLOAT NOT NULL, \
min_DC_power FLOAT NOT NULL, \
time FLOAT NOT NULL, \
avg_f INT unsigned NOT NULL, \
def_f INT unsigned NOT NULL, \
PRIMARY KEY (id))");
    run_query(connection, query);

    sprintf(query, "CREATE TABLE IF NOT EXISTS Events ( \
id INT unsigned NOT NULL AUTO_INCREMENT, \
timestamp INT NOT NULL, \
event_type INT NOT NULL, \
job_id INT unsigned NOT NULL, \
step_id INT unsigned NOT NULL, \
freq INT unsigned NOT NULL, \
node_id VARCHAR(256), \
PRIMARY KEY (id))");
    run_query(connection, query);

    sprintf(query, "CREATE TABLE IF NOT EXISTS Global_energy ( \
energy_percent FLOAT, \
warning_level INT UNSIGNED NOT NULL, \
time TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, \
inc_th FLOAT, \
p_state INT, \
GlobEnergyConsumedT1 INT UNSIGNED, \
GlobEnergyConsumedT2 INT UNSIGNED, \
GlobEnergyLimit INT UNSIGNED, \
GlobEnergyPeriodT1 INT UNSIGNED, \
GlobEnergyPeriodT2 INT UNSIGNED, \
GlobEnergyPolicy VARCHAR(64), \
PRIMARY KEY (time))");
    run_query(connection, query);
                            

    sprintf(query, "CREATE TABLE IF NOT EXISTS Learning_applications (\
job_id INT unsigned NOT NULL, \
step_id INT unsigned NOT NULL, \
node_id VARCHAR(128), \
signature_id INT unsigned,\
power_signature_id INT unsigned, \
PRIMARY KEY(job_id, step_id, node_id))");
    run_query(connection, query);

    sprintf(query, "CREATE TABLE IF NOT EXISTS Learning_jobs (\
id INT unsigned NOT NULL,\
step_id INT unsigned NOT NULL, \
user_id VARCHAR(256),\
app_id VARCHAR(256),\
start_time INT NOT NULL,\
end_time INT NOT NULL,\
start_mpi_time INT NOT NULL,\
end_mpi_time INT NOT NULL,\
policy VARCHAR(256) NOT NULL,\
threshold FLOAT NOT NULL,\
procs INT unsigned NOT NULL,\
job_type SMALLINT unsigned NOT NULL,\
def_f INT unsigned, \
user_acc VARCHAR(256) NOT NULL, \
user_group VARCHAR(256), \
e_tag VARCHAR(256), \
PRIMARY KEY(id, step_id))");
    run_query(connection, query);

    sprintf(query, "CREATE TABLE IF NOT EXISTS Periodic_aggregations (\
id INT unsigned NOT NULL AUTO_INCREMENT,\
start_time INT,\
end_time INT,\
DC_energy INT unsigned, \
eardbd_host VARCHAR(64), \
PRIMARY KEY(id))");
    run_query(connection, query);

    if (signature_detail)
        sprintf(query, "CREATE TABLE IF NOT EXISTS Learning_signatures (\
id INT unsigned NOT NULL AUTO_INCREMENT,\
DC_power FLOAT,\
DRAM_power FLOAT,\
PCK_power FLOAT,\
EDP FLOAT,\
GBS FLOAT,\
TPI FLOAT,\
CPI FLOAT,\
Gflops FLOAT,\
time FLOAT,\
FLOPS1 INT unsigned,\
FLOPS2 INT unsigned,\
FLOPS3 INT unsigned,\
FLOPS4 INT unsigned,\
FLOPS5 INT unsigned,\
FLOPS6 INT unsigned,\
FLOPS7 INT unsigned,\
FLOPS8 INT unsigned,\
instructions INT unsigned, \
cycles INT unsigned,\
avg_f INT unsigned,\
def_f INT unsigned, \
PRIMARY KEY (id))");
    else
        sprintf(query, "CREATE TABLE IF NOT EXISTS Learning_signatures (\
id INT unsigned NOT NULL AUTO_INCREMENT,\
DC_power FLOAT,\
DRAM_power FLOAT,\
PCK_power FLOAT,\
EDP FLOAT,\
GBS FLOAT,\
TPI FLOAT,\
CPI FLOAT,\
Gflops FLOAT,\
time FLOAT,\
avg_f INT unsigned,\
def_f INT unsigned, \
PRIMARY KEY (id))");

    run_query(connection, query);

}


#elif DB_PSQL
void create_tables(void *connection)
{
    char query[1024];
    sprintf(query, "CREATE TABLE IF NOT EXISTS Applications (\
job_id INT  NOT NULL, \
step_id INT  NOT NULL, \
node_id VARCHAR(128), \
signature_id INT , \
power_signature_id INT , \
PRIMARY KEY(job_id, step_id, node_id))");
    run_query(connection, query);

    sprintf(query, "CREATE TABLE IF NOT EXISTS Loops ( \
event INT  NOT NULL, \
size INT  NOT NULL, \
level INT  NOT NULL, \
job_id INT  NOT NULL, \
step_id INT  NOT NULL, \
node_id VARCHAR(8), \
total_iterations INT , \
signature_id INT )");
    run_query(connection, query);

    sprintf(query, "CREATE TABLE IF NOT EXISTS Jobs (\
id INT  NOT NULL,\
step_id INT  NOT NULL, \
user_id VARCHAR(128),\
app_id VARCHAR(128),\
start_time INT NOT NULL,\
end_time INT NOT NULL,\
start_mpi_time INT NOT NULL,\
end_mpi_time INT NOT NULL,\
policy VARCHAR(256) NOT NULL,\
threshold FLOAT NOT NULL,\
procs INT  NOT NULL,\
job_type SMALLINT  NOT NULL,\
def_f INT , \
user_acc VARCHAR(256), \
user_group VARCHAR(256), \
e_tag VARCHAR(256), \
PRIMARY KEY(id, step_id))");
    run_query(connection, query);

    if (signature_detail)
        sprintf(query, "CREATE TABLE IF NOT EXISTS Signatures (\
id SERIAL NOT NULL,\
DC_power FLOAT,\
DRAM_power FLOAT,\
PCK_power FLOAT,\
EDP FLOAT,\
GBS FLOAT,\
TPI FLOAT,\
CPI FLOAT,\
Gflops FLOAT,\
time FLOAT,\
FLOPS1 INT ,\
FLOPS2 INT ,\
FLOPS3 INT ,\
FLOPS4 INT ,\
FLOPS5 INT ,\
FLOPS6 INT ,\
FLOPS7 INT ,\
FLOPS8 INT ,\
instructions INT , \
cycles INT ,\
avg_f INT ,\
def_f INT , \
PRIMARY KEY (id))");
    else
        sprintf(query, "CREATE TABLE IF NOT EXISTS Signatures (\
id SERIAL NOT NULL,\
DC_power FLOAT,\
DRAM_power FLOAT,\
PCK_power FLOAT,\
EDP FLOAT,\
GBS FLOAT,\
TPI FLOAT,\
CPI FLOAT,\
Gflops FLOAT,\
time FLOAT,\
avg_f INT ,\
def_f INT , \
PRIMARY KEY (id))");
    run_query(connection, query);

if (db_node_detail){
    sprintf(query, "CREATE TABLE IF NOT EXISTS Periodic_metrics ( \
id SERIAL NOT NULL, \
start_time INT NOT NULL, \
end_time INT NOT NULL, \
DC_energy INT  NOT NULL, \
node_id VARCHAR(256) NOT NULL, \
job_id INT  NOT NULL, \
step_id INT  NOT NULL, \
avg_f INT, \
temp INT, \
PRIMARY KEY (id))");
}else{
    sprintf(query, "CREATE TABLE IF NOT EXISTS Periodic_metrics ( \
id SERIAL NOT NULL, \
start_time INT NOT NULL, \
end_time INT NOT NULL, \
DC_energy INT  NOT NULL, \
node_id VARCHAR(256) NOT NULL, \
job_id INT  NOT NULL, \
step_id INT  NOT NULL, "
"PRIMARY KEY (id))");
}
    run_query(connection, query);

    sprintf(query, "CREATE TABLE IF NOT EXISTS Power_signatures (  \
id SERIAL NOT NULL, \
DC_power FLOAT NOT NULL, \
DRAM_power FLOAT NOT NULL, \
PCK_power FLOAT NOT NULL, \
EDP FLOAT NOT NULL, \
max_DC_power FLOAT NOT NULL, \
min_DC_power FLOAT NOT NULL, \
time FLOAT NOT NULL, \
avg_f INT  NOT NULL, \
def_f INT  NOT NULL, \
PRIMARY KEY (id))");
    run_query(connection, query);

    sprintf(query, "CREATE TABLE IF NOT EXISTS Events ( \
id SERIAL NOT NULL, \
timestamp INT NOT NULL, \
event_type INT NOT NULL, \
job_id INT  NOT NULL, \
step_id INT  NOT NULL, \
freq INT  NOT NULL, \
node_id VARCHAR(256), \
PRIMARY KEY (id))");
    run_query(connection, query);

    sprintf(query, "CREATE TABLE IF NOT EXISTS Global_energy ( \
energy_percent FLOAT, \
warning_level INT NOT NULL, \
time INT, \
inc_th FLOAT, \
p_state INT, \
GlobEnergyConsumedT1 INT, \
GlobEnergyConsumedT2 INT, \
GlobEnergyLimit INT, \
GlobEnergyPeriodT1 INT, \
GlobEnergyPeriodT2 INT, \
GlobEnergyPolicy VARCHAR(64), \
PRIMARY KEY (time))");
    run_query(connection, query);
                            

    sprintf(query, "CREATE TABLE IF NOT EXISTS Learning_applications (\
job_id INT  NOT NULL, \
step_id INT  NOT NULL, \
node_id VARCHAR(128), \
signature_id INT ,\
power_signature_id INT , \
PRIMARY KEY(job_id, step_id, node_id))");
    run_query(connection, query);

    sprintf(query, "CREATE TABLE IF NOT EXISTS Learning_jobs (\
id INT  NOT NULL,\
step_id INT  NOT NULL, \
user_id VARCHAR(256),\
app_id VARCHAR(256),\
start_time INT NOT NULL,\
end_time INT NOT NULL,\
start_mpi_time INT NOT NULL,\
end_mpi_time INT NOT NULL,\
policy VARCHAR(256) NOT NULL,\
threshold FLOAT NOT NULL,\
procs INT  NOT NULL,\
job_type SMALLINT  NOT NULL,\
def_f INT , \
user_acc VARCHAR(256) NOT NULL, \
user_group VARCHAR(256), \
e_tag VARCHAR(256), \
PRIMARY KEY(id, step_id))");
    run_query(connection, query);

    sprintf(query, "CREATE TABLE IF NOT EXISTS Periodic_aggregations (\
id SERIAL NOT NULL,\
start_time INT,\
end_time INT,\
DC_energy INT , \
eardbd_host VARCHAR(64), \
PRIMARY KEY(id))");
    run_query(connection, query);

    if (signature_detail)
        sprintf(query, "CREATE TABLE IF NOT EXISTS Learning_signatures (\
id SERIAL NOT NULL,\
DC_power FLOAT,\
DRAM_power FLOAT,\
PCK_power FLOAT,\
EDP FLOAT,\
GBS FLOAT,\
TPI FLOAT,\
CPI FLOAT,\
Gflops FLOAT,\
time FLOAT,\
FLOPS1 INT ,\
FLOPS2 INT ,\
FLOPS3 INT ,\
FLOPS4 INT ,\
FLOPS5 INT ,\
FLOPS6 INT ,\
FLOPS7 INT ,\
FLOPS8 INT ,\
instructions INT , \
cycles INT ,\
avg_f INT ,\
def_f INT , \
PRIMARY KEY (id))");
    else
        sprintf(query, "CREATE TABLE IF NOT EXISTS Learning_signatures (\
id SERIAL NOT NULL,\
DC_power FLOAT,\
DRAM_power FLOAT,\
PCK_power FLOAT,\
EDP FLOAT,\
GBS FLOAT,\
TPI FLOAT,\
CPI FLOAT,\
Gflops FLOAT,\
time FLOAT,\
avg_f INT ,\
def_f INT , \
PRIMARY KEY (id))");

    run_query(connection, query);

    sprintf(query, "CREATE OR REPLACE FUNCTION update_time_column() \n\
RETURNS TRIGGER AS $$\n\
BEGIN\n\
NEW.time = now(); \n\
RETURN NEW;\n\
END;\n\
$$ language 'plpgsql'");
    run_query(connection, query);

    sprintf(query, "CREATE TRIGGER update_table_timestamp BEFORE UPDATE ON global_energy FOR EACH ROW EXECUTE PROCEDURE update_time_column()");
    run_query(connection, query);
}
#endif


int main(int argc,char *argv[])
{
    int c;
    char passw[256], root_user[256];;
    if (argc < 2) usage(argv[0]);
#if DB_MYSQL
    strcpy(root_user, "root");
#elif DB_PSQL
    strcpy(root_user, "postgres");
#endif
    
    strcpy(passw, "");

    struct termios t;
    while ((c = getopt(argc, argv, "phrouv")) != -1)
    {
        switch(c)
        {
            case 'h':
               usage(argv[0]);
               break;
            case 'v':
               print_version();
               exit(0);
               break;
            case 'p':
                tcgetattr(STDIN_FILENO, &t);
                t.c_lflag &= ~ECHO;
                tcsetattr(STDIN_FILENO, TCSANOW, &t);

                verbosen(0, "Introduce root's password:");
                fflush(stdout);
                fgets(passw, sizeof(passw), stdin);
                t.c_lflag |= ECHO;
                tcsetattr(STDIN_FILENO, TCSANOW, &t);
                strclean(passw, '\n');
                fprintf(stdout, " ");
                break;
            case 'r':
                break;
            case 'u':
                verbosen(0, "Introduce root's user: ");
                fflush(stdout);
                fgets(root_user, sizeof(root_user), stdin);
                strclean(root_user, '\n');
                fprintf(stdout, " ");
                break;
            case 'o':
                print_out = 1;
                break;
        }
    }


    //cluster_conf_t my_cluster;
    char ear_path[256];

    if (get_ear_conf_path(ear_path) == EAR_ERROR)
    {
        fprintf(stdout, "Error getting ear.conf path"); //error
        exit(0);
    }
#if DB_MYSQL 
    MYSQL *connection = mysql_init(NULL); 
#elif DB_PSQL
    PGconn *connection;
#endif

    read_cluster_conf(ear_path, &my_cluster);

	print_database_conf(&my_cluster.database);

    signature_detail = my_cluster.database.report_sig_detail;
		db_node_detail= my_cluster.database.report_node_detail;

#if DB_PSQL
    char **keys, **values, temp[32];

    keys = calloc(5, sizeof(const char *));
    values = calloc(5, sizeof(const char *));
#endif
    if (!print_out)
    {
#if DB_MYSQL
        if (connection == NULL)
        {
            fprintf(stdout, "Error creating MYSQL object: %s", mysql_error(connection)); //error
            exit(1);
        }

        if (!mysql_real_connect(connection, my_cluster.database.ip, root_user, passw, NULL, my_cluster.database.port, NULL, 0))
        {
            fprintf(stdout, "Error connecting to database: %s", mysql_error(connection)); //error
            free_cluster_conf(&my_cluster);
            exit(1);
        }
#elif DB_PSQL
        sprintf(temp, "%d", my_cluster.database.port);

        keys[0] = "dbname";
        keys[1] = "user";
        keys[2] = "password";
        keys[3] = "host";
        if (my_cluster.database.port > 0)
        {
            keys[4] = "port";
            values[4] = temp;
        }

        strtolow(my_cluster.database.database);
        strtolow(my_cluster.database.ip);

        //values[0] = my_cluster.database.database;
        values[0] = "";
        values[1] = root_user;
        values[2] = passw;
        values[3] = my_cluster.database.ip;

        connection = PQconnectdbParams((const char * const *)keys, (const char * const *)values, 0);

        if (PQstatus(connection) != CONNECTION_OK)
        {
            fprintf(stderr, "ERROR connecting to the database: %s\n", PQerrorMessage(connection));
            free(keys);
            free(values);
            PQfinish(connection);
            free_cluster_conf(&my_cluster);
            exit(1);
        }
#endif
    }

    create_db(connection, my_cluster.database.database);
#if DB_PSQL
    if (!print_out)
    {
        PQfinish(connection);
        values[0] = my_cluster.database.database;
        connection = PQconnectdbParams((const char * const *)keys, (const char * const *)values, 0);
    }

    free(keys);
    free(values);
    if (!print_out && PQstatus(connection) != CONNECTION_OK)
    {
        fprintf(stderr, "ERROR connecting to the database after its creation: %s\n", PQerrorMessage(connection));
        free(keys);
        free(values);
        PQfinish(connection);
        free_cluster_conf(&my_cluster);
        exit(1);
    }
#endif
    
    create_tables(connection);

    create_users(connection, my_cluster.database.database, my_cluster.database.user, my_cluster.database.pass, my_cluster.database.user_commands, my_cluster.database.pass_commands);

    if (!print_out)
    {
        create_indexes(connection);
#if DB_MYSQL
        mysql_close(connection);
#elif DB_PSQL
        PQfinish(connection);
#endif
    }
    free_cluster_conf(&my_cluster);

    if (!print_out) fprintf(stdout, "Database successfully created\n");

    exit(1);
}
