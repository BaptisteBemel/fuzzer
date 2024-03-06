#include <stdio.h>
#include <string.h>

#define TMAGIC "ustar"
#define REGTYPE  '0' 
#define TVERSION "00"

char* path ;

struct tar_t
{                              /* byte offset */
    char name[100];               /*   0 */
    char mode[8];                 /* 100 */
    char uid[8];                  /* 108 */
    char gid[8];                  /* 116 */
    char size[12];                /* 124 */
    char mtime[12];               /* 136 */
    char chksum[8];               /* 148 */
    char typeflag;                /* 156 */
    char linkname[100];           /* 157 */
    char magic[6];                /* 257 */
    char version[2];              /* 263 */
    char uname[32];               /* 265 */
    char gname[32];               /* 297 */
    char devmajor[8];             /* 329 */
    char devminor[8];             /* 337 */
    char prefix[155];             /* 345 */
    char padding[12];             /* 500 */
};

static struct tar_t header;

/**
 * Launches another executable given as argument,
 * parses its output and check whether or not it matches "*** The program has crashed ***".
 * @param the path to the executable
 * @return -1 if the executable cannot be launched,
 *          0 if it is launched but does not print "*** The program has crashed ***",
 *          1 if it is launched and prints "*** The program has crashed ***".
 *
 * BONUS (for fun, no additional marks) without modifying this code,
 * compile it and use the executable to restart our computer.
 */
 int extractor() {

    int rv = 0;
    char cmd[51];
    strncpy(cmd, path, 25);
    cmd[26] = '\0';
    strncat(cmd, " archive.tar", 25);
    char buf[33];
    FILE *fp;

    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Error opening pipe!\n");
        return -1;
    }

    if(fgets(buf, 33, fp) == NULL) {
        printf("No output\n");
        goto finally;
    }
    if(strncmp(buf, "*** The program has crashed ***\n", 33)) {
        printf("Not the crash message\n");
        goto finally;
    } else {
        printf("Crash message\n");
        rv = 1;
        goto finally;
    }
    finally:
    if(pclose(fp) == -1) {
        printf("Command not found\n");
        rv = -1;
    }
    printf("%d\n", rv);
    return rv;
 }

/**
 * Computes the checksum for a tar header and encode it on the header
 * @param entry: The tar header
 * @return the value of the checksum
 */
unsigned int calculate_checksum(struct tar_t* entry){
    // use spaces for the checksum bytes while calculating the checksum
    memset(entry->chksum, ' ', 8);

    // sum of entire metadata
    unsigned int check = 0;
    unsigned char* raw = (unsigned char*) entry;
    for(int i = 0; i < 512; i++){
        check += raw[i];
    }

    snprintf(entry->chksum, sizeof(entry->chksum), "%06o0", check);

    entry->chksum[6] = '\0';
    entry->chksum[7] = ' ';
    return check;
}

void create_header(struct tar_t* header) {
    
    char archive_name[100];
    char linkname[100] = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
    char full_zero[8] = "0000000";

    memset(header, 0, sizeof(struct tar_t));

    snprintf(header->name, sizeof(header->name), "%s", archive_name);
    snprintf(header->mode, sizeof(header->mode), "07777"); // all perms (based on constants.h)
    snprintf(header->uid, sizeof(header->uid),"%s", full_zero);
    snprintf(header->gid, sizeof(header->gid),"%s", full_zero);
    snprintf(header->size, sizeof(header->size), "%011o", 0); // size needs to be in octal
    snprintf(header->mtime, sizeof(header->mtime), "%011lo", time(NULL)); // set modification time to current time in octal format
    //checksum at the end (need all other fields before calculating the checksum)
    header->typeflag = REGTYPE;
    snprintf(header->linkname, sizeof(header->linkname), "%s", linkname);
    snprintf(header->magic, sizeof(header->magic), TMAGIC);
    snprintf(header->version, sizeof(header->version) + 1,  TVERSION);
    snprintf(header->uname, sizeof(header->uname), "Z3US");
    snprintf(header->gname, sizeof(header->gname), "Z3US");
    snprintf(header->devmajor, sizeof(header->devmajor),"%s", full_zero);
    snprintf(header->devminor, sizeof(header->devminor),"%s", full_zero);
}

/**
 * Creates a new tar file
 */
void gen_tar(struct tar_t* header) {
    calculate_checksum(header);

    //Open the tar file 
    FILE *fp = fopen("archive.tar", "wb");
    if (fp == NULL) {
        perror("Error opening file");
    }

    fwrite(header, sizeof(struct tar_t), 1, fp) ;

    char* content_header = NULL ;
    size_t content_header_size = 0 ;
    if (content_header_size > 0 && fwrite(content_header, content_header_size, 1, fp) != 1) {
        perror("Error writing content");
        fclose(fp);
        //exit(EXIT_FAILURE);
    }

    char end_data[1024];
    memset(end_data, 0, 1024);

    if (fwrite(end_data, 1024, 1, fp) != 1) {
        perror("Error writing end bytes");
        fclose(fp); 
        //exit(EXIT_FAILURE);
    }


    //Close the tar file
    if (fclose(fp) != 0) {
        perror("Error closing file");
    }
}

/**
 * Perform fuzzing on various field
 */
void fuzzing() {

    //gen_tar() ;


    create_header(&header);

    gen_tar(&header);

    extractor() ;

}


/*
Main Function
*/
int main(int argc, char* argv[])
{
    
    printf("--- This is a fuzzing test ---\n") ;

    path = argv[1] ;

    fuzzing() ;

    printf("--- End of the fuzzing test ---\n") ;
}