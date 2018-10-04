// To compile : clang -Weverything -fsanitize=address Payload2Header.c -o Payload2Header

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

static int	create_header(char *name)
{
	char	*header_name = NULL;
	char	*ptr = NULL;
	int		fd = 0;

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
	fd = open(header_name, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	check_error("open", fd);

	free(header_name);

	return (fd);
}

static void	convert(int bin, int header)
{
	unsigned char	*buf_read = NULL;
	struct stat		st;
	int				len = 0;
	unsigned char	buf[(13 * 6) + 2] = {0}; // store 13 octets * 6 + 2 for \n\t

	check_error("fstat", fstat(bin, &st));

	// don't need to check if calloc == NULL
	buf_read = (unsigned char *)calloc(sizeof(char), (size_t)st.st_size);

	check_error("read", (int)read(bin, buf_read, (size_t)st.st_size));

	// Write first part
	dprintf(header, "#include <Arduino.h>\n\n#define FUSEE_BIN_SIZE %ld\nconst PROGMEM byte fuseeBin[FUSEE_BIN_SIZE] = {\n\t", st.st_size);

	// write payload in hex
	for (int i = 0; i < st.st_size; i++) {
		if (!(i % 12) && i != 0) {
			len += sprintf((char *)buf + len, "\n\t");
			write(header, &buf, (size_t)len);
			memset(&buf, 0, sizeof(buf));
			len = 0;
		}
		/*dprintf(header, "0x%02x, ", buf_read[i]);*/
		len += sprintf((char *)buf + len, "0x%02x, ", buf_read[i]);
	}
	// write if buffer is not empty
	if (len != 0)
		write(header, &buf, (size_t)len);
	dprintf(header, "\n};\n");
	free(buf_read);
}

int	main(int argc, char **argv)
{
	int		fd_bin = 0;
	int		fd_header = 0;

	if (argc != 2){
		usage(argv[0]);
		return (-1);
	}

	// open file ton convert
	fd_bin = open(argv[1], O_RDONLY);
	check_error("open", fd_bin);

	// create .h
	fd_header = create_header(argv[1]);

	// write payload in file converted
	convert(fd_bin, fd_header);

	close(fd_header);
	close(fd_bin);

	printf("File writed");
	return (0);
}
