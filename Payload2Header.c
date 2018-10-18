// To compile : clang -Weverything Payload2Header.c -o Payload2Header

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static void	usage(char *name)
{
	fprintf(stderr, "Usage: %s payload.bin\n", name);
}

static void	check_error(char *syscall, int error)
{
	if (error == -1) {
		write(2, syscall, strlen(syscall));
		perror(" error");
		exit(errno);
	}
}

static FILE	*create_header(char *name)
{
	char	*header_name = NULL;
	char	*ptr = NULL;
	FILE	*fd = NULL;

	// if name is the entire path, keep only the name
	ptr = strrchr(name, '/');
	if (ptr != NULL) {
		name = ++ptr;
		ptr = NULL;
	}

	//alloc name + ".h" + '\0'
	header_name = (char *)calloc(sizeof(char), strlen(name) + 3);
	strcpy(header_name, name);

	// check if name have ".bin" or other extension
	ptr = strrchr(header_name, '.');
	if (ptr != NULL)
		strcpy(ptr, ".h");
	else
		strcat(header_name, ".h");

	// Create header file
	fd = fopen(header_name, "w+");
	if (fd == NULL) {
		perror("fopen");
		return (NULL);
	}
	/*check_error("open", fd);*/

	free(header_name);

	return (fd);
}

static void	convert(FILE *bin, FILE *header)
{
	unsigned char	*buf_read = NULL;
	long			size = 0;
	int				len = 0;
	unsigned char	buf[(13 * 6) + 2] = {0}; // store 13 octets * 6 + 2 for \n\t

	/*check_error("fstat", fstat(bin, &st));*/
	if (fseek(bin, 0L, SEEK_END) == -1) {
		perror("fseek");
		exit(errno);
	}
	size = ftell(bin);
	rewind(bin);

	// don't need to check if calloc == NULL
	buf_read = (unsigned char *)calloc(sizeof(char), (size_t)size);

	fread(buf_read, (size_t)size, sizeof(char), bin);

	// Write first part
	fprintf(header, "#include <Arduino.h>\n\n#define FUSEE_BIN_SIZE %ld\nconst PROGMEM byte fuseeBin[FUSEE_BIN_SIZE] = {\n\t", size);

	// write payload in hex
	for (int i = 0; i < size; i++) {
		if (!(i % 12) && i != 0) {
			len += sprintf((char *)buf + len, "\n\t");
			fprintf(header, "%s", buf);
			memset(&buf, 0, sizeof(buf));
			len = 0;
		}
		len += sprintf((char *)buf + len, "0x%02x, ", buf_read[i]);
	}
	// write if buffer is not empty
	if (len != 0)
		fprintf(header, "%s", buf);
	fprintf(header, "\n};\n");
	free(buf_read);
}

int	main(int argc, char **argv)
{
	FILE	*fd_header = 0;
	FILE	*fd_bin = NULL;

	if (argc != 2){
		usage(argv[0]);
		return (-1);
	}

	// open file ton convert
	fd_bin = fopen(argv[1], "r");
	if (fd_bin == NULL) {
		perror("fopen");
		return (errno);
	}

	// create .h
	fd_header = create_header(argv[1]);
	if (fd_header == NULL)
		return (errno);

	// write payload in file converted
	convert(fd_bin, fd_header);

	fclose(fd_header);
	fclose(fd_bin);

	printf("File writed\n");
	return (0);
}
