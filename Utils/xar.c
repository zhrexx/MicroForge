#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
#include <errno.h>

#define HEADER_MAGIC "!<arch>\n"
#define FILE_HEADER_SIZE 60

typedef struct {
    char name[16];
    char mtime[12];
    char uid[6];
    char gid[6];
    char mode[8];
    char size[10];
    char end[2];
} FileHeader;

void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s {dtvx}[r] archive-file [file...]\n", prog_name);
    fprintf(stderr, "  d - delete files from archive\n");
    fprintf(stderr, "  t - list contents of archive\n");
    fprintf(stderr, "  v - be verbose\n");
    fprintf(stderr, "  x - extract files from archive\n");
    fprintf(stderr, "  r - append files to archive\n");
    exit(EXIT_FAILURE);
}

void write_header(int fd) {
    if (write(fd, HEADER_MAGIC, 8) != 8) {
        perror("Error writing archive header");
        exit(EXIT_FAILURE);
    }
}

void write_file_header(int fd, const char *filename, size_t size) {
    FileHeader header;
    struct stat st;
    char *name_copy;

    if (stat(filename, &st) == -1) {
        perror("Failed to stat file");
        exit(EXIT_FAILURE);
    }

    memset(&header, ' ', sizeof(header));
    
    name_copy = strdup(filename);
    char *basename = strrchr(name_copy, '/');
    basename = basename ? basename + 1 : name_copy;
    
    snprintf(header.name, sizeof(header.name), "%-15s", basename);
    snprintf(header.mtime, sizeof(header.mtime), "%-11ld", st.st_mtime);
    snprintf(header.uid, sizeof(header.uid), "%-5d", st.st_uid);
    snprintf(header.gid, sizeof(header.gid), "%-5d", st.st_gid);
    snprintf(header.mode, sizeof(header.mode), "%-7o", st.st_mode & 0777);
    snprintf(header.size, sizeof(header.size), "%-9ld", size);
    
    header.end[0] = '`';
    header.end[1] = '\n';
    
    free(name_copy);

    if (write(fd, &header, sizeof(header)) != sizeof(header)) {
        perror("Error writing file header");
        exit(EXIT_FAILURE);
    }
}

void append_files(const char *archive, char **files, int file_count, int verbose) {
    int archive_fd;
    struct stat st;
    
    if (stat(archive, &st) == -1) {
        if (errno == ENOENT) {
            archive_fd = open(archive, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (archive_fd == -1) {
                perror("Failed to create archive");
                exit(EXIT_FAILURE);
            }
            write_header(archive_fd);
        } else {
            perror("Failed to stat archive");
            exit(EXIT_FAILURE);
        }
    } else {
        archive_fd = open(archive, O_WRONLY | O_APPEND);
        if (archive_fd == -1) {
            perror("Failed to open archive for appending");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < file_count; i++) {
        int file_fd = open(files[i], O_RDONLY);
        if (file_fd == -1) {
            fprintf(stderr, "Failed to open %s: %s\n", files[i], strerror(errno));
            continue;
        }
        
        struct stat file_st;
        if (fstat(file_fd, &file_st) == -1) {
            fprintf(stderr, "Failed to stat %s: %s\n", files[i], strerror(errno));
            close(file_fd);
            continue;
        }
        
        if (verbose) {
            printf("a - %s\n", files[i]);
        }
        
        write_file_header(archive_fd, files[i], file_st.st_size);
        
        char buffer[8192];
        ssize_t bytes_read;
        size_t total_bytes = 0;
        
        while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
            if (write(archive_fd, buffer, bytes_read) != bytes_read) {
                perror("Failed to write to archive");
                close(file_fd);
                close(archive_fd);
                exit(EXIT_FAILURE);
            }
            total_bytes += bytes_read;
        }
        
        if (bytes_read == -1) {
            perror("Failed to read from file");
            close(file_fd);
            close(archive_fd);
            exit(EXIT_FAILURE);
        }
        
        if (total_bytes % 2 == 1) {
            if (write(archive_fd, "\n", 1) != 1) {
                perror("Failed to write padding byte");
                close(file_fd);
                close(archive_fd);
                exit(EXIT_FAILURE);
            }
        }
        
        close(file_fd);
    }
    
    close(archive_fd);
}

void list_archive(const char *archive, int verbose) {
    int fd = open(archive, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open archive");
        exit(EXIT_FAILURE);
    }
    
    char magic[8];
    if (read(fd, magic, sizeof(magic)) != sizeof(magic) || memcmp(magic, HEADER_MAGIC, 8) != 0) {
        fprintf(stderr, "%s: Not a valid archive\n", archive);
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    FileHeader header;
    while (read(fd, &header, sizeof(header)) == sizeof(header)) {
        header.name[15] = '\0';
        char name[16];
        sscanf(header.name, "%15s", name);
        
        char size_str[11];
        memcpy(size_str, header.size, 10);
        size_str[10] = '\0';
        size_t size = atol(size_str);
        
        if (verbose) {
            char mode_str[9];
            memcpy(mode_str, header.mode, 8);
            mode_str[8] = '\0';
            
            char mtime_str[13];
            memcpy(mtime_str, header.mtime, 12);
            mtime_str[12] = '\0';
            time_t mtime = atol(mtime_str);
            
            char uid_str[7], gid_str[7];
            memcpy(uid_str, header.uid, 6);
            memcpy(gid_str, header.gid, 6);
            uid_str[6] = gid_str[6] = '\0';
            
            struct tm *tm_info = localtime(&mtime);
            char time_buf[20];
            strftime(time_buf, 20, "%Y-%m-%d %H:%M", tm_info);
            
            printf("%s %5s/%-5s %8ld %s %s\n", 
                   mode_str, uid_str, gid_str, size, time_buf, name);
        } else {
            printf("%s\n", name);
        }
        
        off_t offset = size;
        if (size % 2 == 1) offset++;
        
        if (lseek(fd, offset, SEEK_CUR) == -1) {
            perror("Failed to seek in archive");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    
    close(fd);
}

void extract_files(const char *archive, char **files, int file_count, int verbose) {
    int fd = open(archive, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open archive");
        exit(EXIT_FAILURE);
    }
    
    char magic[8];
    if (read(fd, magic, sizeof(magic)) != sizeof(magic) || memcmp(magic, HEADER_MAGIC, 8) != 0) {
        fprintf(stderr, "%s: Not a valid archive\n", archive);
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    FileHeader header;
    while (read(fd, &header, sizeof(header)) == sizeof(header)) {
        header.name[15] = '\0';
        char name[16];
        sscanf(header.name, "%15s", name);
        
        char size_str[11];
        memcpy(size_str, header.size, 10);
        size_str[10] = '\0';
        size_t size = atol(size_str);
        
        char mode_str[9];
        memcpy(mode_str, header.mode, 8);
        mode_str[8] = '\0';
        mode_t mode = strtol(mode_str, NULL, 8);
        
        int extract = (file_count == 0);
        for (int i = 0; i < file_count && !extract; i++) {
            if (strcmp(files[i], name) == 0) {
                extract = 1;
            }
        }
        
        if (extract) {
            if (verbose) {
                printf("x - %s\n", name);
            }
            
            int out_fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, mode);
            if (out_fd == -1) {
                fprintf(stderr, "Failed to create %s: %s\n", name, strerror(errno));
                off_t offset = size;
                if (size % 2 == 1) offset++;
                if (lseek(fd, offset, SEEK_CUR) == -1) {
                    perror("Failed to seek in archive");
                    close(fd);
                    exit(EXIT_FAILURE);
                }
                continue;
            }
            
            size_t remaining = size;
            char buffer[8192];
            while (remaining > 0) {
                size_t to_read = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
                ssize_t bytes_read = read(fd, buffer, to_read);
                
                if (bytes_read <= 0) {
                    perror("Failed to read from archive");
                    close(out_fd);
                    close(fd);
                    exit(EXIT_FAILURE);
                }
                
                if (write(out_fd, buffer, bytes_read) != bytes_read) {
                    perror("Failed to write to file");
                    close(out_fd);
                    close(fd);
                    exit(EXIT_FAILURE);
                }
                
                remaining -= bytes_read;
            }
            
            close(out_fd);
            
            if (size % 2 == 1) {
                if (lseek(fd, 1, SEEK_CUR) == -1) {
                    perror("Failed to seek in archive");
                    close(fd);
                    exit(EXIT_FAILURE);
                }
            }
        } else {
            off_t offset = size;
            if (size % 2 == 1) offset++;
            
            if (lseek(fd, offset, SEEK_CUR) == -1) {
                perror("Failed to seek in archive");
                close(fd);
                exit(EXIT_FAILURE);
            }
        }
    }
    
    close(fd);
}

void delete_files(const char *archive, char **files, int file_count, int verbose) {
    if (file_count == 0) {
        fprintf(stderr, "No files specified for deletion\n");
        exit(EXIT_FAILURE);
    }
    
    char temp_name[256];
    snprintf(temp_name, sizeof(temp_name), "%s.tmp", archive);
    
    int in_fd = open(archive, O_RDONLY);
    if (in_fd == -1) {
        perror("Failed to open archive");
        exit(EXIT_FAILURE);
    }
    
    int out_fd = open(temp_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd == -1) {
        perror("Failed to create temporary archive");
        close(in_fd);
        exit(EXIT_FAILURE);
    }
    
    char magic[8];
    if (read(in_fd, magic, sizeof(magic)) != sizeof(magic) || memcmp(magic, HEADER_MAGIC, 8) != 0) {
        fprintf(stderr, "%s: Not a valid archive\n", archive);
        close(in_fd);
        close(out_fd);
        unlink(temp_name);
        exit(EXIT_FAILURE);
    }
    
    write_header(out_fd);
    
    FileHeader header;
    while (read(in_fd, &header, sizeof(header)) == sizeof(header)) {
        header.name[15] = '\0';
        char name[16];
        sscanf(header.name, "%15s", name);
        
        char size_str[11];
        memcpy(size_str, header.size, 10);
        size_str[10] = '\0';
        size_t size = atol(size_str);
        
        int delete = 0;
        for (int i = 0; i < file_count; i++) {
            if (strcmp(files[i], name) == 0) {
                delete = 1;
                if (verbose) {
                    printf("d - %s\n", name);
                }
                break;
            }
        }
        
        if (!delete) {
            if (write(out_fd, &header, sizeof(header)) != sizeof(header)) {
                perror("Failed to write file header");
                close(in_fd);
                close(out_fd);
                unlink(temp_name);
                exit(EXIT_FAILURE);
            }
            
            char buffer[8192];
            size_t remaining = size;
            
            while (remaining > 0) {
                size_t to_read = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
                ssize_t bytes_read = read(in_fd, buffer, to_read);
                
                if (bytes_read <= 0) {
                    perror("Failed to read from archive");
                    close(in_fd);
                    close(out_fd);
                    unlink(temp_name);
                    exit(EXIT_FAILURE);
                }
                
                if (write(out_fd, buffer, bytes_read) != bytes_read) {
                    perror("Failed to write to temporary archive");
                    close(in_fd);
                    close(out_fd);
                    unlink(temp_name);
                    exit(EXIT_FAILURE);
                }
                
                remaining -= bytes_read;
            }
            
            if (size % 2 == 1) {
                if (write(out_fd, "\n", 1) != 1) {
                    perror("Failed to write padding byte");
                    close(in_fd);
                    close(out_fd);
                    unlink(temp_name);
                    exit(EXIT_FAILURE);
                }
                if (lseek(in_fd, 1, SEEK_CUR) == -1) {
                    perror("Failed to seek in archive");
                    close(in_fd);
                    close(out_fd);
                    unlink(temp_name);
                    exit(EXIT_FAILURE);
                }
            }
        } else {
            off_t offset = size;
            if (size % 2 == 1) offset++;
            
            if (lseek(in_fd, offset, SEEK_CUR) == -1) {
                perror("Failed to seek in archive");
                close(in_fd);
                close(out_fd);
                unlink(temp_name);
                exit(EXIT_FAILURE);
            }
        }
    }
    
    close(in_fd);
    close(out_fd);
    
    if (rename(temp_name, archive) == -1) {
        perror("Failed to replace archive with temporary file");
        unlink(temp_name);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
    }
    
    const char *options = argv[1];
    const char *archive = argv[2];
    
    if (options[0] == '\0') {
        fprintf(stderr, "No operation specified\n");
        print_usage(argv[0]);
    }
    
    int delete_op = 0, list_op = 0, verbose_op = 0, extract_op = 0, append_op = 0;
    
    for (size_t i = 0; i < strlen(options); i++) {
        switch (options[i]) {
            case 'd':
                delete_op = 1;
                break;
            case 't':
                list_op = 1;
                break;
            case 'v':
                verbose_op = 1;
                break;
            case 'x':
                extract_op = 1;
                break;
            case 'r':
                append_op = 1;
                break;
            default:
                fprintf(stderr, "Unknown option: %c\n", options[i]);
                print_usage(argv[0]);
        }
    }
    
    if ((delete_op + list_op + extract_op + append_op) != 1) {
        fprintf(stderr, "Exactly one operation (d, t, x, r) must be specified\n");
        print_usage(argv[0]);
    }
    
    char **files = &argv[3];
    int file_count = argc - 3;
    
    if (append_op && file_count == 0) {
        fprintf(stderr, "No files specified for append operation\n");
        print_usage(argv[0]);
    }
    
    if (delete_op) {
        delete_files(archive, files, file_count, verbose_op);
    } else if (list_op) {
        list_archive(archive, verbose_op);
    } else if (extract_op) {
        extract_files(archive, files, file_count, verbose_op);
    } else if (append_op) {
        append_files(archive, files, file_count, verbose_op);
    }
    
    return 0;
}
