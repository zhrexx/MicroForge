#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>

#define NUM_REPOS 1
#define CMD_SIZE 1024

typedef struct {
    char repo_url[256];
    char dir[128];
    int clone_status;
    int update_status;
    int build_status;
    double build_time;
    char last_commit[256];
    char branch[64];
    char error_output[512];
    char details[2048];
} BuildResult;

void getDirName(const char *repo_url, char *dir, size_t size) {
    const char *lastSlash = strrchr(repo_url, '/');
    if (lastSlash) {
        strncpy(dir, lastSlash + 1, size);
    } else {
        strncpy(dir, repo_url, size);
    }
    char *dot = strstr(dir, ".git");
    if (dot) *dot = '\0';
}

int dirExists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

int runCommand(const char *command) {
    int ret = system(command);
    return ret == -1 ? -1 : WEXITSTATUS(ret);
}

void getCommandOutput(const char *command, char *output, size_t size) {
    FILE *fp = popen(command, "r");
    if (fp) {
        fgets(output, size, fp);
        pclose(fp);
    } else {
        strncpy(output, "N/A", size);
    }
}

void getCommandFullOutput(const char *command, char *output, size_t size) {
    FILE *fp = popen(command, "r");
    if (fp) {
        size_t total = 0;
        while (fgets(output + total, size - total, fp) != NULL && total < size - 1) {
            total = strlen(output);
        }
        pclose(fp);
    } else {
        strncpy(output, "N/A", size);
    }
}

int main(void) {
    BuildResult results[NUM_REPOS];
    const char *repos[NUM_REPOS] = {
        "https://github.com/zhrexx/MicroForge.git",
    };
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    for (int i = 0; i < NUM_REPOS; i++) {
        memset(&results[i], 0, sizeof(BuildResult));
        strncpy(results[i].repo_url, repos[i], sizeof(results[i].repo_url));
        getDirName(repos[i], results[i].dir, sizeof(results[i].dir));
        snprintf(results[i].details, sizeof(results[i].details),
                 "<strong>Repo:</strong> %s<br><strong>Dir:</strong> %s<br>",
                 results[i].repo_url, results[i].dir);
        if (!dirExists(results[i].dir)) {
            char cmd[CMD_SIZE];
            snprintf(cmd, sizeof(cmd), "git clone %s 2>&1", results[i].repo_url);
            strcat(results[i].details, "<strong>Cloning:</strong> ");
            strcat(results[i].details, cmd);
            strcat(results[i].details, "<br>");
            results[i].clone_status = runCommand(cmd);
            char buf[128];
            snprintf(buf, sizeof(buf), "Code: %d<br>", results[i].clone_status);
            strcat(results[i].details, buf);
            if (results[i].clone_status != 0) {
                getCommandFullOutput(cmd, results[i].error_output, sizeof(results[i].error_output));
                strcat(results[i].details, "<strong>Error:</strong> ");
                strcat(results[i].details, results[i].error_output);
                strcat(results[i].details, "<br>");
                results[i].update_status = -1;
                results[i].build_status = -1;
                continue;
            }
        } else {
            results[i].clone_status = 0;
            strcat(results[i].details, "Already cloned.<br>");
            char cmd[CMD_SIZE];
            snprintf(cmd, sizeof(cmd), "cd %s && git pull --rebase 2>&1", results[i].dir);
            strcat(results[i].details, "<strong>Updating:</strong> ");
            strcat(results[i].details, cmd);
            strcat(results[i].details, "<br>");
            results[i].update_status = runCommand(cmd);
            char buf[64];
            snprintf(buf, sizeof(buf), "Code: %d<br>", results[i].update_status);
            strcat(results[i].details, buf);
            if (results[i].update_status != 0) {
                getCommandFullOutput(cmd, results[i].error_output, sizeof(results[i].error_output));
                strcat(results[i].details, "<strong>Error:</strong> ");
                strcat(results[i].details, results[i].error_output);
                strcat(results[i].details, "<br>");
            }
        }
        if (dirExists(results[i].dir)) {
            char cmd[CMD_SIZE];
            char commit[256] = {0};
            snprintf(cmd, sizeof(cmd), "cd %s && git log -1 --pretty=format:\"%%h %%s\" 2>&1", results[i].dir);
            getCommandOutput(cmd, commit, sizeof(commit));
            strncpy(results[i].last_commit, commit, sizeof(results[i].last_commit));
            char branch[64] = {0};
            snprintf(cmd, sizeof(cmd), "cd %s && git branch --show-current 2>&1", results[i].dir);
            getCommandOutput(cmd, branch, sizeof(branch));
            strncpy(results[i].branch, branch, sizeof(results[i].branch));
        } else {
            strncpy(results[i].last_commit, "N/A", sizeof(results[i].last_commit));
            strncpy(results[i].branch, "N/A", sizeof(results[i].branch));
        }
        char cmd[CMD_SIZE];
        snprintf(cmd, sizeof(cmd), "cd %s && make -B 2>&1", results[i].dir);
        strcat(results[i].details, "<strong>Building:</strong> ");
        strcat(results[i].details, cmd);
        strcat(results[i].details, "<br>");
        time_t start = time(NULL);
        results[i].build_status = runCommand(cmd);
        time_t end = time(NULL);
        results[i].build_time = difftime(end, start);
        char buf[128];
        snprintf(buf, sizeof(buf), "Code: %d<br><strong>Time:</strong> %.0f sec<br>",
                 results[i].build_status, results[i].build_time);
        strcat(results[i].details, buf);
        if (results[i].build_status != 0) {
            getCommandFullOutput(cmd, results[i].error_output, sizeof(results[i].error_output));
            strcat(results[i].details, "<strong>Error:</strong> ");
            strcat(results[i].details, results[i].error_output);
            strcat(results[i].details, "<br>");
        }
    }
    if (mkdir("checks", 0755) != 0 && errno != EEXIST) {
        perror("mkdir checks");
        exit(1);
    }
    FILE *htmlFile = fopen("checks/build_report.html", "w");
    if (!htmlFile) { perror("fopen build_report.html"); exit(1); }
    fprintf(htmlFile,
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"    <meta charset=\"UTF-8\">\n"
"    <title>Build Dashboard</title>\n"
"    <link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">\n"
"    <script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>\n"
"</head>\n"
"<body>\n"
"    <div class=\"container\">\n"
"        <h1>Build Dashboard</h1>\n"
"        <p>Updated: %s</p>\n"
"        <div class=\"topControls\">\n"
"            <input type=\"text\" id=\"search\" placeholder=\"Search repository...\" onkeyup=\"searchRepo()\" />\n"
"            <select id=\"statusFilter\" onchange=\"filterStatus()\">\n"
"                <option value=\"all\">All</option>\n"
"                <option value=\"success\">Success</option>\n"
"                <option value=\"failure\">Failure</option>\n"
"            </select>\n"
"            <button class=\"button\" onclick=\"window.location.reload();\">Refresh</button>\n"
"            <button class=\"button\" id=\"autoRefreshBtn\" onclick=\"toggleAutoRefresh()\">Start Auto-Refresh</button>\n"
"            <button class=\"button\" onclick=\"exportCSV()\">Export CSV</button>\n"
"        </div>\n"
"        <div class=\"chartContainer\">\n"
"            <canvas id=\"statusChart\"></canvas>\n"
"        </div>\n"
"        <table id=\"resultsTable\">\n"
"            <thead>\n"
"            <tr>\n"
"                <th onclick=\"sortTable(0)\">Repo</th>\n"
"                <th onclick=\"sortTable(1)\">Dir</th>\n"
"                <th onclick=\"sortTable(2)\">Branch</th>\n"
"                <th onclick=\"sortTable(3)\">Clone</th>\n"
"                <th onclick=\"sortTable(4)\">Update</th>\n"
"                <th onclick=\"sortTable(5)\">Build</th>\n"
"                <th onclick=\"sortTable(6)\">Time</th>\n"
"                <th onclick=\"sortTable(7)\">Last Commit</th>\n"
"                <th>Details</th>\n"
"            </tr>\n"
"            </thead>\n"
"            <tbody>\n", timestamp);
    for (int i = 0; i < NUM_REPOS; i++) {
        const char *cloneClass = (results[i].clone_status == 0 ? "success" : "failure");
        const char *updateClass = (results[i].update_status == 0 ? "success" : "failure");
        const char *buildClass = (results[i].build_status == 0 ? "success" : "failure");
        fprintf(htmlFile, "            <tr class=\"repoRow\" data-status=\"%s\">\n",
                (results[i].clone_status==0 && results[i].update_status==0 && results[i].build_status==0) ? "success" : "failure");
        fprintf(htmlFile, "                <td title=\"%s\">%s</td>\n", results[i].repo_url, results[i].repo_url);
        fprintf(htmlFile, "                <td>%s</td>\n", results[i].dir);
        fprintf(htmlFile, "                <td>%s</td>\n", results[i].branch[0] ? results[i].branch : "N/A");
        fprintf(htmlFile, "                <td class=\"%s\">%s</td>\n", cloneClass, (results[i].clone_status==0 ? "OK" : "Fail"));
        if (results[i].update_status == -1) {
            fprintf(htmlFile, "                <td class=\"failure\">N/A</td>\n");
        } else {
            fprintf(htmlFile, "                <td class=\"%s\">%s</td>\n", updateClass, (results[i].update_status==0 ? "OK" : "Fail"));
        }
        if (results[i].build_status == -1) {
            fprintf(htmlFile, "                <td class=\"failure\">N/A</td>\n");
            fprintf(htmlFile, "                <td>N/A</td>\n");
        } else {
            fprintf(htmlFile, "                <td class=\"%s\">%s</td>\n", buildClass, (results[i].build_status==0 ? "OK" : "Fail"));
            fprintf(htmlFile, "                <td>%.0f</td>\n", results[i].build_time);
        }
        fprintf(htmlFile, "                <td>%s</td>\n", results[i].last_commit[0] ? results[i].last_commit : "N/A");
        fprintf(htmlFile, "                <td><button class=\"button\" onclick=\"toggleDetails('detail%d')\">Toggle</button></td>\n", i);
        fprintf(htmlFile, "            </tr>\n");
        fprintf(htmlFile, "            <tr id=\"detail%d\" class=\"detailsRow\" style=\"display:none;\"><td colspan=\"9\">%s</td></tr>\n", i, results[i].details);
    }
    fprintf(htmlFile,
"            </tbody>\n"
"        </table>\n"
"        <div class=\"navButtons\">\n"
"            <a href=\"detailed_report.html\"><button class=\"button\">Detailed Report</button></a>\n"
"        </div>\n"
"    </div>\n"
"    <script src=\"script.js\"></script>\n"
"</body>\n"
"</html>\n");
    fclose(htmlFile);
    FILE *detailFile = fopen("checks/detailed_report.html", "w");
    if (!detailFile) { perror("fopen detailed_report.html"); exit(1); }
    fprintf(detailFile,
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"    <meta charset=\"UTF-8\">\n"
"    <title>Detailed Build Report</title>\n"
"    <link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">\n"
"</head>\n"
"<body>\n"
"    <div class=\"container\">\n"
"        <h1>Detailed Build Report</h1>\n"
"        <p>Updated: %s</p>\n"
"        <div class=\"detailContainer\">\n", timestamp);
    for (int i = 0; i < NUM_REPOS; i++) {
        fprintf(detailFile,
"            <div class=\"detailBlock\">\n"
"                <h2>%s</h2>\n"
"                <p><strong>Repo URL:</strong> %s</p>\n"
"                <p><strong>Directory:</strong> %s</p>\n"
"                <p><strong>Branch:</strong> %s</p>\n"
"                <p><strong>Clone:</strong> %s</p>\n"
"                <p><strong>Update:</strong> %s</p>\n"
"                <p><strong>Build:</strong> %s</p>\n"
"                <p><strong>Time:</strong> %.0f sec</p>\n"
"                <p><strong>Last Commit:</strong> %s</p>\n"
"                <div class=\"detailInfo\">\n"
"                    <p>%s</p>\n"
"                </div>\n"
"            </div>\n",
            results[i].repo_url,
            results[i].repo_url,
            results[i].dir,
            results[i].branch[0] ? results[i].branch : "N/A",
            (results[i].clone_status==0 ? "OK" : "Fail"),
            (results[i].update_status==0 ? "OK" : (results[i].update_status==-1 ? "N/A" : "Fail")),
            (results[i].build_status==0 ? "OK" : (results[i].build_status==-1 ? "N/A" : "Fail")),
            results[i].build_time,
            results[i].last_commit[0] ? results[i].last_commit : "N/A",
            results[i].details);
    }
    fprintf(detailFile,
"        </div>\n"
"        <div class=\"navButtons\">\n"
"            <a href=\"build_report.html\"><button class=\"button\">Back to Dashboard</button></a>\n"
"        </div>\n"
"    </div>\n"
"    <script src=\"script.js\"></script>\n"
"</body>\n"
"</html>\n");
    fclose(detailFile);
    FILE *cssFile = fopen("checks/style.css", "w");
    if (!cssFile) { perror("fopen style.css"); exit(1); }
    fprintf(cssFile,
"body {font-family: Arial, sans-serif; background-color: #1e1e1e; color: #fff; margin: 0; padding: 20px;}\n"
".container {width: 90%%; margin: auto; background: #2a2a2a; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(255,255,255,0.2); text-align: center;}\n"
".topControls {margin-bottom: 15px;}\n"
".topControls input, .topControls select {padding: 8px; margin-right: 10px; border-radius: 5px; border: 1px solid #555;}\n"
".chartContainer {width: 30%%; margin: 20px auto;}\n"
"table {width: 100%%; border-collapse: collapse; margin-top: 20px;}\n"
"th, td {padding: 10px; border: 1px solid #555; text-align: center;}\n"
"th {background-color: #444; cursor: pointer;}\n"
".success {background-color: #4caf50; color: #fff;}\n"
".failure {background-color: #f44336; color: #fff;}\n"
".button {background-color: #008cba; color: #fff; padding: 8px 16px; margin: 5px; border: none; cursor: pointer; border-radius: 5px; font-size: 14px;}\n"
".button:hover {background-color: #005f73;}\n"
".detailsRow td {background-color: #333; text-align: left;}\n"
".navButtons {margin-top: 20px;}\n"
".detailContainer {display: flex; flex-wrap: wrap; justify-content: space-around;}\n"
".detailBlock {background: #333; border: 1px solid #555; border-radius: 5px; padding: 15px; margin: 10px; width: 45%%; text-align: left;}\n"
".detailBlock h2 {margin-top: 0;}\n"
".detailInfo {background: #222; padding: 10px; border-radius: 5px; font-size: 0.9em;}\n");
    fclose(cssFile);
    FILE *jsFile = fopen("checks/script.js", "w");
    if (!jsFile) { perror("fopen script.js"); exit(1); }
    fprintf(jsFile,
"var autoRefreshInterval=null;\n"
"function toggleDetails(id){var elem=document.getElementById(id);elem.style.display=(elem.style.display==='none')?'table-row':'none';}\n"
"function sortTable(n){var table=document.getElementById('resultsTable'),rows,switching=true,i,x,y,shouldSwitch,dir='asc',switchcount=0;while(switching){switching=false;rows=table.rows;for(i=1;i<(rows.length-1);i++){shouldSwitch=false;x=rows[i].getElementsByTagName('TD')[n];y=rows[i+1].getElementsByTagName('TD')[n];if(dir=='asc'){if(x.innerHTML.toLowerCase()>y.innerHTML.toLowerCase()){shouldSwitch=true;break;}}else if(dir=='desc'){if(x.innerHTML.toLowerCase()<y.innerHTML.toLowerCase()){shouldSwitch=true;break;}}}if(shouldSwitch){rows[i].parentNode.insertBefore(rows[i+1],rows[i]);switching=true;switchcount++;}else{if(switchcount==0&&dir=='asc'){dir='desc';switching=true;}}}}\n"
"function searchRepo(){var input=document.getElementById('search');var filter=input.value.toUpperCase();var table=document.getElementById('resultsTable');var tr=table.getElementsByClassName('repoRow');for(var i=0;i<tr.length;i++){var td=tr[i].getElementsByTagName('td')[0];if(td){var txtValue=td.textContent||td.innerText;tr[i].style.display=(txtValue.toUpperCase().indexOf(filter)>-1)?'':'none';}}}\n"
"function filterStatus(){var select=document.getElementById('statusFilter');var filter=select.value;var rows=document.getElementsByClassName('repoRow');for(var i=0;i<rows.length;i++){rows[i].style.display=(filter==='all')?'':(rows[i].getAttribute('data-status')===filter?'':'none');}}\n"
"function toggleAutoRefresh(){var btn=document.getElementById('autoRefreshBtn');if(autoRefreshInterval==null){autoRefreshInterval=setInterval(function(){window.location.reload();},30000);btn.innerText='Stop Auto-Refresh';}else{clearInterval(autoRefreshInterval);autoRefreshInterval=null;btn.innerText='Start Auto-Refresh';}}\n"
"function exportCSV(){var csv='Repo,Dir,Branch,Clone,Update,Build,Time,Last Commit\\n';var rows=document.getElementById('resultsTable').rows;for(var i=1;i<rows.length;i+=2){var cols=rows[i].getElementsByTagName('td');if(cols.length>0){csv+=cols[0].innerText+','+cols[1].innerText+','+cols[2].innerText+','+cols[3].innerText+','+cols[4].innerText+','+cols[5].innerText+','+cols[6].innerText+','+cols[7].innerText+'\\n';}}var hiddenElement=document.createElement('a');hiddenElement.href='data:text/csv;charset=utf-8,'+encodeURI(csv);hiddenElement.target='_blank';hiddenElement.download='build_report.csv';hiddenElement.click();}\n"
"window.onload=function(){var ctx=document.getElementById('statusChart').getContext('2d');var rows=document.getElementsByClassName('repoRow');var successCount=0,failureCount=0;for(var i=0;i<rows.length;i++){if(rows[i].getAttribute('data-status')==='success'){successCount++;}else{failureCount++;}}var data={labels:['Success','Failure'],datasets:[{data:[successCount,failureCount],backgroundColor:['#4caf50','#f44336']}]};new Chart(ctx,{type:'pie',data:data,options:{responsive:true,legend:{position:'bottom'}}});};\n");
    fclose(jsFile);
    printf("Build check complete. Reports saved in 'checks'.\n");
    return 0;
}

