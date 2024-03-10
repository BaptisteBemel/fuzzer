#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>

/**
 * Values taken from the Tar Format manual
*/

#define TMAGIC "ustar"
#define REGTYPE  '0' 
#define TVERSION "00"

#define TSUID    04000          /* set UID on execution */
#define TSGID    02000          /* set GID on execution */
#define TSVTX    01000          /* reserved */
                                /* file permissions */
#define TUREAD   00400          /* read by owner */
#define TUWRITE  00200          /* write by owner */
#define TUEXEC   00100          /* execute/search by owner */
#define TGREAD   00040          /* read by group */
#define TGWRITE  00020          /* write by group */
#define TGEXEC   00010          /* execute/search by group */
#define TOREAD   00004          /* read by other */
#define TOWRITE  00002          /* write by other */
#define TOEXEC   00001          /* execute/search by other */

char* path ;
int number_of_success = 1 ;

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

        //Rename file
        char new_name[200];
        snprintf(new_name, sizeof(new_name), "success_%d.tar", number_of_success);

        rename("archive.tar", new_name);
        number_of_success++ ;

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


/**
 * Initiate the header to a starting state wich will be the same for almost all the test
 * @param header: The tar header
*/
void create_header(struct tar_t* header) {
    
    char archive_name[100];
    snprintf(archive_name, sizeof(archive_name), "archive.tar");
    char linkname[100] = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
    char full_zero[8] = "0000000";

    memset(header, 0, sizeof(struct tar_t));

    snprintf(header->name, sizeof(header->name), "%s", archive_name);
    snprintf(header->mode, sizeof(header->mode), "07777");
    snprintf(header->uid, sizeof(header->uid),"%s", full_zero);
    snprintf(header->gid, sizeof(header->gid),"%s", full_zero);
    snprintf(header->size, sizeof(header->size), "%011o", 0); // size needs to be in octal
    snprintf(header->mtime, sizeof(header->mtime), "%011lo", time(NULL));
    header->typeflag = REGTYPE;
    snprintf(header->linkname, sizeof(header->linkname), "%s", linkname);
    snprintf(header->magic, sizeof(header->magic), TMAGIC);
    snprintf(header->version, sizeof(header->version) + 1,  TVERSION);
    snprintf(header->uname, sizeof(header->uname), "myuser");
    snprintf(header->gname, sizeof(header->gname), "myuser");
    snprintf(header->devmajor, sizeof(header->devmajor),"%s", full_zero);
    snprintf(header->devminor, sizeof(header->devminor),"%s", full_zero);
}

/**
 * Creates a new tar file with the header given as argument
 * @param header: The tar header
 */
void gen_tar(struct tar_t* header) {
    calculate_checksum(header);

    //Open the tar file 
    FILE *fp = fopen("archive.tar", "wb");
    if (fp == NULL) {
        perror("Error opening file");
    }

    fwrite(header, sizeof(struct tar_t), 1, fp) ;

    char end_data[1024];
    memset(end_data, 0, 1024);

    if (fwrite(end_data, 1024, 1, fp) != 1) {
        perror("Error writing end bytes");
        fclose(fp); 
    }

    //Close the tar file
    if (fclose(fp) != 0) {
        perror("Error closing file");
    }
}


/**
 * Perform different tests on a specific field of the header
 * @param field_to_fuze: The field from the header wich will be changes when performing the tests
 * @param size_field_to_fuze: The size of the specific field
 */
void fuzzing(char* field_to_fuze, size_t size_field_to_fuze) {

    //Test 1
    //Simple string
    create_header(&header);
    strncpy(field_to_fuze,"SimpleString",size_field_to_fuze);
    gen_tar(&header);
    extractor() ;

    //Test 2
    //Null field
    create_header(&header);
    memset(field_to_fuze,0,size_field_to_fuze);
    gen_tar(&header);
    extractor() ;

    //Test 3
    //Empty Field
    create_header(&header);
    strncpy(field_to_fuze,"",size_field_to_fuze);
    gen_tar(&header);
    extractor() ;

    //Test 4
    //field too long
    create_header(&header);
    strncpy(field_to_fuze,"ThisIsAFieldIHopeIsReallyLongAndTooLongForTheExtractorToWorkCorrectly",size_field_to_fuze);
    gen_tar(&header);
    extractor();

    //Test 5
    //Non ascii char
    create_header(&header);
    strncpy(field_to_fuze,"é",size_field_to_fuze);
    gen_tar(&header);
    extractor();

    //Test 5
    //Control char
    create_header(&header);
    strncpy(field_to_fuze,"\n",size_field_to_fuze);
    gen_tar(&header);
    extractor();

    //Test 6
    //Test with special characters
    create_header(&header);
    strncpy(field_to_fuze, "!@#$%^&*()_+-=[]{};':,.<>?/\\|~`", size_field_to_fuze);
    gen_tar(&header);
    extractor();

    //Test 7
    //Only integer
    create_header(&header);
    memset(field_to_fuze,5,size_field_to_fuze);
    gen_tar(&header);
    extractor();

    //Test 8
    //Negative number
    create_header(&header);
    memset(field_to_fuze,-5,size_field_to_fuze);
    gen_tar(&header);
    extractor();
}

/**
 * Fuzzing on the field name
*/
void name(){
    printf(" Start of Fuzzing on NAME\n");

    fuzzing(header.name, sizeof(header.name));

    printf(" End of Fuzzing on MODE\n");
}

/**
 * Fuzzing on the field mode
*/
void mode(){
    printf(" Start of Fuzzing on MODE\n");

    fuzzing(header.mode, sizeof(header.mode));

    int modes[] = {TSUID, TSGID, TSVTX, TUREAD, TUWRITE, TUEXEC, TGREAD, TGWRITE, TGEXEC, TOREAD, TOWRITE, TOEXEC};

    for(int ii=0; ii < sizeof(modes); ii++){
        char mode[sizeof(header.mode)];
        create_header(&header);
        snprintf(mode, sizeof(header.mode), "%o", modes[ii]);
        strncpy(header.mode, mode, sizeof(header.mode));
        gen_tar(&header);
        extractor();
    }

    printf(" End of Fuzzing on MODE\n");
}

//Fuzzing on the field uid - c'est tout
void uid(){
    printf(" Start of Fuzzing on UID\n");

    fuzzing(header.uid, sizeof(header.uid));

    printf(" End of Fuzzing on UID\n");
}

//Fuzzing on the field gid - c'est tout
void gid(){
    printf(" Start of Fuzzing on GID\n");

    fuzzing(header.gid, sizeof(header.gid));

    printf(" End of Fuzzing on GID\n");
}

//Fuzzing on the field size
void size(){
    printf(" Start of Fuzzing on SIZE\n");

    fuzzing(header.size, sizeof(header.size));

    srand(time(NULL)) ;

    //Fuzzing with 10 random size , can be changed
    for (int i = 0 ; i<10 ; i++) {
        int random_size = rand() % 512 ; //512 = blocksize
        create_header(&header);
        snprintf(header.size,sizeof(header.size), "%o",random_size); //segementation core dumped with strncpy
        gen_tar(&header);
        extractor() ;
    }

    //With negative size
    //INT_MIN = -2147483648
    create_header(&header);
    snprintf(header.size,sizeof(header.size), "%d", INT_MIN); //segementation core dumped with strncpy
    gen_tar(&header);
    extractor() ;

    printf(" End of Fuzzing on SIZE\n");
}


/**
 * Fuzz on the field mtime with the time given as paramater
 * @param time: var to put in mtime field from the header 
*/
void mtime_fuzz(time_t time){
    char time_string[sizeof(header.mtime)] ;
    create_header(&header);
    snprintf(time_string, sizeof(header.mtime), "%lo", INT_MIN);
    strncpy(header.mtime,time_string,sizeof(header.mtime));
    gen_tar(&header);
    extractor() ;
}

//Fuzzing on the field mtime
void mtime(){
    printf(" Start of Fuzzing on MTIME\n");

    fuzzing(header.mtime, sizeof(header.mtime));

    //Various test on mtime
    mtime_fuzz(INT_MIN) ;
    mtime_fuzz(-10) ;
    mtime_fuzz(10) ;
    mtime_fuzz(time(NULL)) ;
    mtime_fuzz(time(NULL)+INT_MAX) ;


    printf(" End of Fuzzing on MTIME\n");
    
}

//Fuzzing on the field chksum
void chksum(){
    printf(" Start of Fuzzing on CHKSUM\n");

    fuzzing(header.chksum, sizeof(header.chksum));

    printf(" End of Fuzzing on CHKSUM\n");
    
}

//Fuzzing on the field typeflag
void typeflag(){

    printf(" Start of Fuzzing on TYPEFLAG\n");

    // Brute-force every possible value from -1 to 255, negative and all possible positive value
    for (int i = -1; i < 256; i++){
        create_header(&header);
        header.typeflag = (char) i;
        gen_tar(&header);
        extractor() ;
    }

    printf(" End of Fuzzing on TYPEFLAG\n");
}

//Fuzzing on the field linkname - c'est tout
void linkname(){
    printf(" Start of Fuzzing on LINKNAME\n");

    fuzzing(header.linkname, sizeof(header.linkname));

    printf(" End of Fuzzing on LINKNAME\n");
    
}

//Fuzzing on the field magic - c'est tout
void magic(){
    printf(" Start of Fuzzing on MAGIC\n");

    fuzzing(header.magic, sizeof(header.magic));

    printf(" End of Fuzzing on MAGIC\n");
    
}

//Fuzzing on the field version
void version(){
    printf(" Start of Fuzzing on VERSION\n");

    fuzzing(header.version, sizeof(header.version));

    //Rajouter bruteforce car seulement 2bits (en octal) pour ce champs
    for (int i = 0; i < 10; i++) {
        for (int j = 0 ; j < 10 ; j++) {
            int version[2] = {i,j} ;
            create_header(&header);
            strncpy(header.version,version,sizeof(header.version));
            gen_tar(&header);
            extractor() ;
        }
    }
    
    printf(" End of Fuzzing on VERSION\n");
    
}

//Fuzzing on the field uname - c'est tout
void uname(){
    printf(" Start of Fuzzing on UNAME\n");

    fuzzing(header.uname, sizeof(header.uname));

    printf(" End of Fuzzing on UNAME\n");
    
}

//Fuzzing on the field gname - c'est tout
void gname(){
    printf(" Start of Fuzzing on GNAME\n");

    fuzzing(header.gname, sizeof(header.gname));

    printf(" End of Fuzzing on GNAME\n");
    
}


/**
 * Main Function
 * Calls the different functions to perform the fuzzing on the header
 * @param argv[1]: contains the path to the extractor
*/
int main(int argc, char* argv[])
{

    if (argc != 2) {
        printf("Invalid number of arguments.\n");
        printf("Usage : ./fuzzer <path to extractor>");
        return -1;
    }

    path = argv[1] ;
    
    printf("--- This is a fuzzing test ---\n") ;

    //name() ;
    mode() ;
    uid() ;
    gid() ;
    size() ;
    mtime();
    chksum();
    typeflag() ;
    linkname();
    magic() ;
    version();
    uname();
    gname();
    

    printf("\n--- End of the fuzzing test ---\n") ;
}
