#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_PATH_LENGTH 1024
#define MAX_FILES 1000
#define TEMPLATE_SIZE 8192

typedef struct {
    char name[256];
    char path[MAX_PATH_LENGTH];
    long size;
    char modified[32];
    int is_dir;
} FileEntry;

void get_file_details(const char *base_path, const char *file_name, FileEntry *entry) {
    struct stat file_stat;
    char full_path[MAX_PATH_LENGTH];
    
    snprintf(full_path, sizeof(full_path), "%s/%s", base_path, file_name);
    strcpy(entry->name, file_name);
    strcpy(entry->path, full_path);
    
    if (stat(full_path, &file_stat) == 0) {
        entry->size = file_stat.st_size;
        entry->is_dir = S_ISDIR(file_stat.st_mode);
        strftime(entry->modified, sizeof(entry->modified), "%Y-%m-%d %H:%M:%S", 
                 localtime(&file_stat.st_mtime));
    } else {
        entry->size = 0;
        entry->is_dir = 0;
        strcpy(entry->modified, "Unknown");
    }
}

int compare_files_by_name(const void *a, const void *b) {
    const FileEntry *fa = (const FileEntry *)a;
    const FileEntry *fb = (const FileEntry *)b;
    
    if (fa->is_dir && !fb->is_dir) return -1;
    if (!fa->is_dir && fb->is_dir) return 1;
    
    return strcasecmp(fa->name, fb->name);
}

int compare_files_by_size(const void *a, const void *b) {
    const FileEntry *fa = (const FileEntry *)a;
    const FileEntry *fb = (const FileEntry *)b;
    
    if (fa->is_dir && !fb->is_dir) return -1;
    if (!fa->is_dir && fb->is_dir) return 1;
    
    return (fa->size > fb->size) - (fa->size < fb->size);
}

int compare_files_by_date(const void *a, const void *b) {
    const FileEntry *fa = (const FileEntry *)a;
    const FileEntry *fb = (const FileEntry *)b;
    
    if (fa->is_dir && !fb->is_dir) return -1;
    if (!fa->is_dir && fb->is_dir) return 1;
    
    return strcmp(fb->modified, fa->modified);
}

void generate_index_page(const char *directory, const char *output_file, int sort_type) {
    DIR *dir;
    struct dirent *entry;
    FileEntry files[MAX_FILES];
    int file_count = 0;
    char path[MAX_PATH_LENGTH];
    char parent_path[MAX_PATH_LENGTH];
    
    strcpy(path, directory);
    
    if ((dir = opendir(path)) == NULL) {
        perror("Unable to open directory");
        return;
    }
    
    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        if (strcmp(entry->d_name, ".") == 0) continue;
        
        get_file_details(path, entry->d_name, &files[file_count]);
        file_count++;
    }
    
    closedir(dir);
    
    switch (sort_type) {
        case 1: 
            qsort(files, file_count, sizeof(FileEntry), compare_files_by_name);
            break;
        case 2: 
            qsort(files, file_count, sizeof(FileEntry), compare_files_by_size);
            break;
        case 3: 
            qsort(files, file_count, sizeof(FileEntry), compare_files_by_date);
            break;
        default: 
            qsort(files, file_count, sizeof(FileEntry), compare_files_by_name);
    }
    
    FILE *fp = fopen(output_file, "w");
    if (fp == NULL) {
        perror("Unable to create output file");
        return;
    }
    
    strcpy(parent_path, directory);
    char *last_slash = strrchr(parent_path, '/');
    if (last_slash != NULL) {
        *last_slash = '\0';
    } else {
        strcpy(parent_path, ".");
    }
    
    fprintf(fp, "<!DOCTYPE html>\n");
    fprintf(fp, "<html lang='en'>\n");
    fprintf(fp, "<head>\n");
    fprintf(fp, "    <meta charset='UTF-8'>\n");
    fprintf(fp, "    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n");
    fprintf(fp, "    <title>Index of %s</title>\n", directory);
    fprintf(fp, "    <style>\n");
    fprintf(fp, "        :root {\n");
    fprintf(fp, "            --bg-color: #2a2a2e;\n");
    fprintf(fp, "            --text-color: #f9f9fa;\n");
    fprintf(fp, "            --accent-color: #0060df;\n");
    fprintf(fp, "            --hover-color: #0a84ff;\n");
    fprintf(fp, "            --border-color: #4a4a4f;\n");
    fprintf(fp, "            --row-hover: #35353b;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        body {\n");
    fprintf(fp, "            font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;\n");
    fprintf(fp, "            background-color: var(--bg-color);\n");
    fprintf(fp, "            color: var(--text-color);\n");
    fprintf(fp, "            margin: 0;\n");
    fprintf(fp, "            padding: 20px;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        .container {\n");
    fprintf(fp, "            max-width: 1200px;\n");
    fprintf(fp, "            margin: 0 auto;\n");
    fprintf(fp, "            border-radius: 8px;\n");
    fprintf(fp, "            overflow: hidden;\n");
    fprintf(fp, "            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);\n");
    fprintf(fp, "            background-color: #32323a;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        header {\n");
    fprintf(fp, "            background-color: var(--accent-color);\n");
    fprintf(fp, "            padding: 15px 20px;\n");
    fprintf(fp, "            display: flex;\n");
    fprintf(fp, "            justify-content: space-between;\n");
    fprintf(fp, "            align-items: center;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        header h1 {\n");
    fprintf(fp, "            margin: 0;\n");
    fprintf(fp, "            font-size: 1.5rem;\n");
    fprintf(fp, "            font-weight: 500;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        .controls {\n");
    fprintf(fp, "            display: flex;\n");
    fprintf(fp, "            gap: 10px;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        .controls select {\n");
    fprintf(fp, "            background-color: rgba(255, 255, 255, 0.15);\n");
    fprintf(fp, "            color: white;\n");
    fprintf(fp, "            border: none;\n");
    fprintf(fp, "            padding: 5px 10px;\n");
    fprintf(fp, "            border-radius: 4px;\n");
    fprintf(fp, "            cursor: pointer;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        .controls button {\n");
    fprintf(fp, "            background-color: rgba(255, 255, 255, 0.15);\n");
    fprintf(fp, "            color: white;\n");
    fprintf(fp, "            border: none;\n");
    fprintf(fp, "            padding: 5px 10px;\n");
    fprintf(fp, "            border-radius: 4px;\n");
    fprintf(fp, "            cursor: pointer;\n");
    fprintf(fp, "            transition: background-color 0.2s;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        .controls button:hover {\n");
    fprintf(fp, "            background-color: rgba(255, 255, 255, 0.25);\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        .search-bar {\n");
    fprintf(fp, "            padding: 10px 20px;\n");
    fprintf(fp, "            background-color: #42424a;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        #search {\n");
    fprintf(fp, "            width: 100%%;\n");
    fprintf(fp, "            padding: 8px 12px;\n");
    fprintf(fp, "            border-radius: 4px;\n");
    fprintf(fp, "            border: 1px solid var(--border-color);\n");
    fprintf(fp, "            background-color: #2a2a2e;\n");
    fprintf(fp, "            color: var(--text-color);\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        table {\n");
    fprintf(fp, "            width: 100%%;\n");
    fprintf(fp, "            border-collapse: collapse;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        th {\n");
    fprintf(fp, "            padding: 12px 20px;\n");
    fprintf(fp, "            text-align: left;\n");
    fprintf(fp, "            background-color: #42424a;\n");
    fprintf(fp, "            position: sticky;\n");
    fprintf(fp, "            top: 0;\n");
    fprintf(fp, "            cursor: pointer;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        th:hover {\n");
    fprintf(fp, "            background-color: #4a4a54;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        td {\n");
    fprintf(fp, "            padding: 10px 20px;\n");
    fprintf(fp, "            border-bottom: 1px solid var(--border-color);\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        tr:hover {\n");
    fprintf(fp, "            background-color: var(--row-hover);\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        a {\n");
    fprintf(fp, "            color: var(--text-color);\n");
    fprintf(fp, "            text-decoration: none;\n");
    fprintf(fp, "            display: block;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        a:hover {\n");
    fprintf(fp, "            color: var(--hover-color);\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        .folder {\n");
    fprintf(fp, "            color: #45a1ff;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        .folder:before {\n");
    fprintf(fp, "            content: '📁 ';\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        .file:before {\n");
    fprintf(fp, "            content: '📄 ';\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        .size, .date {\n");
    fprintf(fp, "            text-align: right;\n");
    fprintf(fp, "            white-space: nowrap;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        footer {\n");
    fprintf(fp, "            text-align: center;\n");
    fprintf(fp, "            padding: 15px;\n");
    fprintf(fp, "            background-color: #32323a;\n");
    fprintf(fp, "            color: #b1b1b3;\n");
    fprintf(fp, "            font-size: 0.9rem;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        .theme-switcher {\n");
    fprintf(fp, "            display: flex;\n");
    fprintf(fp, "            justify-content: center;\n");
    fprintf(fp, "            margin-top: 10px;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        .light-theme {\n");
    fprintf(fp, "            --bg-color: #f9f9fa;\n");
    fprintf(fp, "            --text-color: #0c0c0d;\n");
    fprintf(fp, "            --border-color: #d7d7db;\n");
    fprintf(fp, "            --row-hover: #e7e7e7;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        @media (max-width: 768px) {\n");
    fprintf(fp, "            .date {\n");
    fprintf(fp, "                display: none;\n");
    fprintf(fp, "            }\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "    </style>\n");
    fprintf(fp, "</head>\n");
    fprintf(fp, "<body>\n");
    fprintf(fp, "    <div class='container'>\n");
    fprintf(fp, "        <header>\n");
    fprintf(fp, "            <h1>Index of %s</h1>\n", directory);
    fprintf(fp, "            <div class='controls'>\n");
    fprintf(fp, "                <select id='view-mode'>\n");
    fprintf(fp, "                    <option value='list'>List View</option>\n");
    fprintf(fp, "                    <option value='grid'>Grid View</option>\n");
    fprintf(fp, "                </select>\n");
    fprintf(fp, "                <button id='theme-toggle'>Toggle Theme</button>\n");
    fprintf(fp, "            </div>\n");
    fprintf(fp, "        </header>\n");
    fprintf(fp, "        <div class='search-bar'>\n");
    fprintf(fp, "            <input type='text' id='search' placeholder='Search files and folders...'>\n");
    fprintf(fp, "        </div>\n");
    fprintf(fp, "        <div class='table-container'>\n");
    fprintf(fp, "            <table>\n");
    fprintf(fp, "                <thead>\n");
    fprintf(fp, "                    <tr>\n");
    fprintf(fp, "                        <th data-sort='name'>Name</th>\n");
    fprintf(fp, "                        <th data-sort='size' class='size'>Size</th>\n");
    fprintf(fp, "                        <th data-sort='date' class='date'>Last Modified</th>\n");
    fprintf(fp, "                    </tr>\n");
    fprintf(fp, "                </thead>\n");
    fprintf(fp, "                <tbody>\n");
    
    fprintf(fp, "                    <tr>\n");
    fprintf(fp, "                        <td><a href='%s' class='folder'>..</a></td>\n", parent_path);
    fprintf(fp, "                        <td class='size'>-</td>\n");
    fprintf(fp, "                        <td class='date'>-</td>\n");
    fprintf(fp, "                    </tr>\n");
    
    for (int i = 0; i < file_count; i++) {
        char file_class[10];
        char size_str[32];
        
        if (files[i].is_dir) {
            strcpy(file_class, "folder");
            strcpy(size_str, "-");
        } else {
            strcpy(file_class, "file");
            if (files[i].size < 1024) {
                sprintf(size_str, "%ld B", files[i].size);
            } else if (files[i].size < 1024 * 1024) {
                sprintf(size_str, "%.1f KB", files[i].size / 1024.0);
            } else if (files[i].size < 1024 * 1024 * 1024) {
                sprintf(size_str, "%.1f MB", files[i].size / (1024.0 * 1024.0));
            } else {
                sprintf(size_str, "%.1f GB", files[i].size / (1024.0 * 1024.0 * 1024.0));
            }
        }
        
        fprintf(fp, "                    <tr>\n");
        fprintf(fp, "                        <td><a href='%s' class='%s'>%s</a></td>\n", 
                files[i].path, file_class, files[i].name);
        fprintf(fp, "                        <td class='size'>%s</td>\n", size_str);
        fprintf(fp, "                        <td class='date'>%s</td>\n", files[i].modified);
        fprintf(fp, "                    </tr>\n");
    }
    
    fprintf(fp, "                </tbody>\n");
    fprintf(fp, "            </table>\n");
    fprintf(fp, "        </div>\n");
    fprintf(fp, "        <footer>\n");
    fprintf(fp, "            <p>Generated on %s</p>\n", __DATE__);
    fprintf(fp, "            <div class='theme-switcher'>\n");
    fprintf(fp, "                <button id='theme-toggle-bottom'>Switch Theme</button>\n");
    fprintf(fp, "            </div>\n");
    fprintf(fp, "        </footer>\n");
    fprintf(fp, "    </div>\n");
    fprintf(fp, "    <script>\n");
    fprintf(fp, "        document.addEventListener('DOMContentLoaded', function() {\n");
    fprintf(fp, "            const themeToggle = document.getElementById('theme-toggle');\n");
    fprintf(fp, "            const themeToggleBottom = document.getElementById('theme-toggle-bottom');\n");
    fprintf(fp, "            const body = document.body;\n");
    fprintf(fp, "            const viewMode = document.getElementById('view-mode');\n");
    fprintf(fp, "            const searchInput = document.getElementById('search');\n");
    fprintf(fp, "            const tableHeaders = document.querySelectorAll('th[data-sort]');\n");
    fprintf(fp, "            const tableRows = document.querySelectorAll('tbody tr');\n");
    fprintf(fp, "            \n");
    fprintf(fp, "            function toggleTheme() {\n");
    fprintf(fp, "                body.classList.toggle('light-theme');\n");
    fprintf(fp, "                const isLightTheme = body.classList.contains('light-theme');\n");
    fprintf(fp, "                localStorage.setItem('lightTheme', isLightTheme);\n");
    fprintf(fp, "            }\n");
    fprintf(fp, "            \n");
    fprintf(fp, "            if (localStorage.getItem('lightTheme') === 'true') {\n");
    fprintf(fp, "                body.classList.add('light-theme');\n");
    fprintf(fp, "            }\n");
    fprintf(fp, "            \n");
    fprintf(fp, "            themeToggle.addEventListener('click', toggleTheme);\n");
    fprintf(fp, "            themeToggleBottom.addEventListener('click', toggleTheme);\n");
    fprintf(fp, "            \n");
    fprintf(fp, "            searchInput.addEventListener('input', function() {\n");
    fprintf(fp, "                const searchTerm = this.value.toLowerCase();\n");
    fprintf(fp, "                \n");
    fprintf(fp, "                tableRows.forEach(row => {\n");
    fprintf(fp, "                    const fileName = row.querySelector('a').textContent.toLowerCase();\n");
    fprintf(fp, "                    if (fileName === '..') {\n");
    fprintf(fp, "                        row.style.display = '';\n");
    fprintf(fp, "                        return;\n");
    fprintf(fp, "                    }\n");
    fprintf(fp, "                    \n");
    fprintf(fp, "                    if (fileName.includes(searchTerm)) {\n");
    fprintf(fp, "                        row.style.display = '';\n");
    fprintf(fp, "                    } else {\n");
    fprintf(fp, "                        row.style.display = 'none';\n");
    fprintf(fp, "                    }\n");
    fprintf(fp, "                });\n");
    fprintf(fp, "            });\n");
    fprintf(fp, "            \n");
    fprintf(fp, "            let currentSort = { column: 'name', direction: 'asc' };\n");
    fprintf(fp, "            \n");
    fprintf(fp, "            function sortTable(column) {\n");
    fprintf(fp, "                const tableBody = document.querySelector('tbody');\n");
    fprintf(fp, "                const rows = Array.from(tableRows);\n");
    fprintf(fp, "                const parentRow = rows.shift();\n");
    fprintf(fp, "                \n");
    fprintf(fp, "                if (currentSort.column === column) {\n");
    fprintf(fp, "                    currentSort.direction = currentSort.direction === 'asc' ? 'desc' : 'asc';\n");
    fprintf(fp, "                } else {\n");
    fprintf(fp, "                    currentSort.column = column;\n");
    fprintf(fp, "                    currentSort.direction = 'asc';\n");
    fprintf(fp, "                }\n");
    fprintf(fp, "                \n");
    fprintf(fp, "                rows.sort((a, b) => {\n");
    fprintf(fp, "                    const aIsFolder = a.querySelector('a').classList.contains('folder');\n");
    fprintf(fp, "                    const bIsFolder = b.querySelector('a').classList.contains('folder');\n");
    fprintf(fp, "                    \n");
    fprintf(fp, "                    if (aIsFolder && !bIsFolder) return -1;\n");
    fprintf(fp, "                    if (!aIsFolder && bIsFolder) return 1;\n");
    fprintf(fp, "                    \n");
    fprintf(fp, "                    let aValue, bValue;\n");
    fprintf(fp, "                    \n");
    fprintf(fp, "                    if (column === 'name') {\n");
    fprintf(fp, "                        aValue = a.querySelector('a').textContent.toLowerCase();\n");
    fprintf(fp, "                        bValue = b.querySelector('a').textContent.toLowerCase();\n");
    fprintf(fp, "                    } else if (column === 'size') {\n");
    fprintf(fp, "                        const aSizeText = a.querySelector('.size').textContent;\n");
    fprintf(fp, "                        const bSizeText = b.querySelector('.size').textContent;\n");
    fprintf(fp, "                        \n");
    fprintf(fp, "                        if (aSizeText === '-') aValue = 0;\n");
    fprintf(fp, "                        else {\n");
    fprintf(fp, "                            const aSizeVal = parseFloat(aSizeText);\n");
    fprintf(fp, "                            if (aSizeText.includes('KB')) aValue = aSizeVal * 1024;\n");
    fprintf(fp, "                            else if (aSizeText.includes('MB')) aValue = aSizeVal * 1024 * 1024;\n");
    fprintf(fp, "                            else if (aSizeText.includes('GB')) aValue = aSizeVal * 1024 * 1024 * 1024;\n");
    fprintf(fp, "                            else aValue = aSizeVal;\n");
    fprintf(fp, "                        }\n");
    fprintf(fp, "                        \n");
    fprintf(fp, "                        if (bSizeText === '-') bValue = 0;\n");
    fprintf(fp, "                        else {\n");
    fprintf(fp, "                            const bSizeVal = parseFloat(bSizeText);\n");
    fprintf(fp, "                            if (bSizeText.includes('KB')) bValue = bSizeVal * 1024;\n");
    fprintf(fp, "                            else if (bSizeText.includes('MB')) bValue = bSizeVal * 1024 * 1024;\n");
    fprintf(fp, "                            else if (bSizeText.includes('GB')) bValue = bSizeVal * 1024 * 1024 * 1024;\n");
    fprintf(fp, "                            else bValue = bSizeVal;\n");
    fprintf(fp, "                        }\n");
    fprintf(fp, "                    } else if (column === 'date') {\n");
    fprintf(fp, "                        aValue = new Date(a.querySelector('.date').textContent);\n");
    fprintf(fp, "                        bValue = new Date(b.querySelector('.date').textContent);\n");
    fprintf(fp, "                    }\n");
    fprintf(fp, "                    \n");
    fprintf(fp, "                    if (currentSort.direction === 'asc') {\n");
    fprintf(fp, "                        return aValue > bValue ? 1 : -1;\n");
    fprintf(fp, "                    } else {\n");
    fprintf(fp, "                        return aValue < bValue ? 1 : -1;\n");
    fprintf(fp, "                    }\n");
    fprintf(fp, "                });\n");
    fprintf(fp, "                \n");
    fprintf(fp, "                while (tableBody.firstChild) {\n");
    fprintf(fp, "                    tableBody.removeChild(tableBody.firstChild);\n");
    fprintf(fp, "                }\n");
    fprintf(fp, "                \n");
    fprintf(fp, "                tableBody.appendChild(parentRow);\n");
    fprintf(fp, "                \n");
    fprintf(fp, "                rows.forEach(row => {\n");
    fprintf(fp, "                    tableBody.appendChild(row);\n");
    fprintf(fp, "                });\n");
    fprintf(fp, "            }\n");
    fprintf(fp, "            \n");
    fprintf(fp, "            tableHeaders.forEach(header => {\n");
    fprintf(fp, "                header.addEventListener('click', function() {\n");
    fprintf(fp, "                    const column = this.getAttribute('data-sort');\n");
    fprintf(fp, "                    sortTable(column);\n");
    fprintf(fp, "                });\n");
    fprintf(fp, "            });\n");
    fprintf(fp, "            \n");
    fprintf(fp, "            viewMode.addEventListener('change', function() {\n");
    fprintf(fp, "                const tableContainer = document.querySelector('.table-container');\n");
    fprintf(fp, "                if (this.value === 'grid') {\n");
    fprintf(fp, "                    tableContainer.innerHTML = `\n");
    fprintf(fp, "                        <div class='grid-view'>\n");
    fprintf(fp, "                            <div class='grid-item parent-dir'>\n");
    fprintf(fp, "                                <a href='%s' class='folder'>..</a>\n", parent_path);
    fprintf(fp, "                            </div>\n");
    
    for (int i = 0; i < file_count; i++) {
        char file_class[10];
        
        if (files[i].is_dir) {
            strcpy(file_class, "folder");
        } else {
            strcpy(file_class, "file");
        }
        
        fprintf(fp, "                            <div class='grid-item'>\n");
        fprintf(fp, "                                <a href='%s' class='%s'>%s</a>\n", 
                files[i].path, file_class, files[i].name);
        fprintf(fp, "                            </div>\n");
    }
    
    fprintf(fp, "                        </div>\n");
    fprintf(fp, "                    `;\n");
    fprintf(fp, "                    \n");
    fprintf(fp, "                    const style = document.createElement('style');\n");
    fprintf(fp, "                    style.id = 'grid-styles';\n");
    fprintf(fp, "                    style.textContent = `\n");
    fprintf(fp, "                        .grid-view {\n");
    fprintf(fp, "                            display: grid;\n");
    fprintf(fp, "                            grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));\n");
    fprintf(fp, "                            gap: 15px;\n");
    fprintf(fp, "                            padding: 20px;\n");
    fprintf(fp, "                        }\n");
    fprintf(fp, "                        .grid-item {\n");
    fprintf(fp, "                            background-color: #42424a;\n");
    fprintf(fp, "                            border-radius: 6px;\n");
    fprintf(fp, "                            padding: 15px;\n");
    fprintf(fp, "                            text-align: center;\n");
    fprintf(fp, "                            transition: transform 0.2s, background-color 0.2s;\n");
    fprintf(fp, "                        }\n");
    fprintf(fp, "                        .grid-item:hover {\n");
    fprintf(fp, "                            background-color: var(--row-hover);\n");
    fprintf(fp, "                            transform: translateY(-3px);\n");
    fprintf(fp, "                        }\n");
    fprintf(fp, "                        .grid-item a {\n");
    fprintf(fp, "                            display: flex;\n");
    fprintf(fp, "                            flex-direction: column;\n");
    fprintf(fp, "                            align-items: center;\n");
    fprintf(fp, "                            height: 100%%;\n");
    fprintf(fp, "                        }\n");
    fprintf(fp, "                        .grid-item a:before {\n");
    fprintf(fp, "                            font-size: 2rem;\n");
    fprintf(fp, "                            margin-bottom: 10px;\n");
    fprintf(fp, "                        }\n");
    fprintf(fp, "                        .parent-dir {\n");
    fprintf(fp, "                            background-color: var(--accent-color);\n");
    fprintf(fp, "                        }\n");
    fprintf(fp, "                        .light-theme .grid-item {\n");
    fprintf(fp, "                            background-color: #e0e0e6;\n");
    fprintf(fp, "                        }\n");
    fprintf(fp, "                        .light-theme .parent-dir {\n");
    fprintf(fp, "                            background-color: var(--accent-color);\n");
    fprintf(fp, "                        }\n");
    fprintf(fp, "                    `;\n");
    fprintf(fp, "                    document.head.appendChild(style);\n");
    fprintf(fp, "                } else {\n");
    fprintf(fp, "                    const gridStyles = document.getElementById('grid-styles');\n");
    fprintf(fp, "                    if (gridStyles) gridStyles.remove();\n");
    fprintf(fp, "                    \n");
    fprintf(fp, "                    tableContainer.innerHTML = `\n");
    fprintf(fp, "                        <table>\n");
    fprintf(fp, "                            <thead>\n");
    fprintf(fp, "                                <tr>\n");
    fprintf(fp, "                                    <th data-sort='name'>Name</th>\n");
    fprintf(fp, "                                    <th data-sort='size' class='size'>Size</th>\n");
    fprintf(fp, "                                    <th data-sort='date' class='date'>Last Modified</th>\n");
    fprintf(fp, "                                </tr>\n");
    fprintf(fp, "                            </thead>\n");
    fprintf(fp, "                            <tbody>\n");
    fprintf(fp, "                                <tr>\n");
    fprintf(fp, "                                    <td><a href='%s' class='folder'>..</a></td>\n", parent_path);
    fprintf(fp, "                                    <td class='size'>-</td>\n");
    fprintf(fp, "                                    <td class='date'>-</td>\n");
    fprintf(fp, "                                </tr>\n");
    
    for (int i = 0; i < file_count; i++) {
        char file_class[10];
        char size_str[32];
        
        if (files[i].is_dir) {
            strcpy(file_class, "folder");
            strcpy(size_str, "-");
        } else {
            strcpy(file_class, "file");
            if (files[i].size < 1024) {
                sprintf(size_str, "%ld B", files[i].size);
            } else if (files[i].size < 1024 * 1024) {
                sprintf(size_str, "%.1f KB", files[i].size / 1024.0);
            } else if (files[i].size < 1024 * 1024 * 1024) {
                sprintf(size_str, "%.1f MB", files[i].size / (1024.0 * 1024.0));
            } else {
                sprintf(size_str, "%.1f GB", files[i].size / (1024.0 * 1024.0 * 1024.0));
            }
        }
        
        fprintf(fp, "                                <tr>\n");
        fprintf(fp, "                                    <td><a href='%s' class='%s'>%s</a></td>\n", 
                files[i].path, file_class, files[i].name);
        fprintf(fp, "                                    <td class='size'>%s</td>\n", size_str);
        fprintf(fp, "                                    <td class='date'>%s</td>\n", files[i].modified);
        fprintf(fp, "                                </tr>\n");
    }
    
    fprintf(fp, "                            </tbody>\n");
    fprintf(fp, "                        </table>\n");
    fprintf(fp, "                    `;\n");
    fprintf(fp, "                    \n");
    fprintf(fp, "                    const newTableHeaders = document.querySelectorAll('th[data-sort]');\n");
    fprintf(fp, "                    newTableHeaders.forEach(header => {\n");
    fprintf(fp, "                        header.addEventListener('click', function() {\n");
    fprintf(fp, "                            const column = this.getAttribute('data-sort');\n");
    fprintf(fp, "                            sortTable(column);\n");
    fprintf(fp, "                        });\n");
    fprintf(fp, "                    });\n");
    fprintf(fp, "                }\n");
    fprintf(fp, "                \n");
    fprintf(fp, "                localStorage.setItem('viewMode', this.value);\n");
    fprintf(fp, "            });\n");
    fprintf(fp, "            \n");
    fprintf(fp, "            const savedViewMode = localStorage.getItem('viewMode');\n");
    fprintf(fp, "            if (savedViewMode) {\n");
    fprintf(fp, "                viewMode.value = savedViewMode;\n");
    fprintf(fp, "                viewMode.dispatchEvent(new Event('change'));\n");
    fprintf(fp, "            }\n");
    fprintf(fp, "        });\n");
    fprintf(fp, "    </script>\n");
    fprintf(fp, "</body>\n");
    fprintf(fp, "</html>\n");

    fclose(fp);
    printf("Index page generated successfully: %s\n", output_file);
}

int main(int argc, char *argv[]) {
    char directory[MAX_PATH_LENGTH] = ".";
    char output_file[MAX_PATH_LENGTH] = "index.html";
    int sort_type = 1; 
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
            if (i + 1 < argc) {
                strcpy(directory, argv[i + 1]);
                i++;
            }
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                strcpy(output_file, argv[i + 1]);
                i++;
            }
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--sort") == 0) {
            if (i + 1 < argc) {
                if (strcmp(argv[i + 1], "name") == 0) {
                    sort_type = 1;
                } else if (strcmp(argv[i + 1], "size") == 0) {
                    sort_type = 2;
                } else if (strcmp(argv[i + 1], "date") == 0) {
                    sort_type = 3;
                }
                i++;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  -d, --dir DIR       Specify directory to index (default: current directory)\n");
            printf("  -o, --output FILE   Specify output file (default: index.html)\n");
            printf("  -s, --sort TYPE     Sort by: name, size, date (default: name)\n");
            printf("  -h, --help          Show this help message\n");
            return 0;
        }
    }
    
    generate_index_page(directory, output_file, sort_type);
    
    return 0;
}
