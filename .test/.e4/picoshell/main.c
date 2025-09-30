
// EJEMPLO DE USO CON MAIN:

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int picoshell(char **cmds[]);

static int count_cmds(int argc, char **argv)
{
    int count = 1;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "|") == 0)
            count++;
    }
    return count;
}

int main(int argc, char **argv)
{
    if (argc < 2)
        return (fprintf(stderr, "Usage: %s cmd1 [args] | cmd2 [args] ...\n", argv[0]), 1);

    int cmd_count = count_cmds(argc, argv);
    char ***cmds = calloc(cmd_count + 1, sizeof(char **));
    if (!cmds)
        return (perror("calloc"), 1);

    // Parsear argumentos y construir array de comandos
    int i = 1, j = 0;
    while (i < argc)
    {
        int len = 0;
        while (i + len < argc && strcmp(argv[i + len], "|") != 0)
            len++;

        cmds[j] = calloc(len + 1, sizeof(char *));
        if (!cmds[j])
            return (perror("calloc"), 1);

        for (int k = 0; k < len; k++)
            cmds[j][k] = argv[i + k];
        cmds[j][len] = NULL;

        i += len + 1;  // Saltar el "|"
        j++;
    }
    cmds[cmd_count] = NULL;

    int ret = picoshell(cmds);

    // Limpiar memoria
    for (int i = 0; cmds[i]; i++)
        free(cmds[i]);
    free(cmds);

    return ret;
}


/*
 * DIAGRAMA DE PIPELINE PARA "ls | grep txt | wc -l":
 *
 * COMANDO 1: ls
 * -------------
 * HIJO 1:  stdin=terminal  stdout=pipe1[1]  stderr=terminal
 * PADRE:   close(pipe1[1]) prev_fd=pipe1[0]
 *
 * COMANDO 2: grep txt
 * --------------------
 * HIJO 2:  stdin=pipe1[0]  stdout=pipe2[1]  stderr=terminal
 * PADRE:   close(pipe1[0]) close(pipe2[1]) prev_fd=pipe2[0]
 *
 * COMANDO 3: wc -l
 * ----------------
 * HIJO 3:  stdin=pipe2[0]  stdout=terminal  stderr=terminal
 * PADRE:   close(pipe2[0]) wait_all()
 *
 * FLUJO DE DATOS:
 * ls → pipe1 → grep → pipe2 → wc → terminal
 */

/*
 * GESTIÓN DETALLADA DE FILE DESCRIPTORS:
 *
 * Iteración 1 (ls):
 * - Crear pipe1
 * - Fork hijo1
 * - Hijo1: dup2(pipe1[1], STDOUT), exec ls
 * - Padre: close(pipe1[1]), prev_fd = pipe1[0]
 *
 * Iteración 2 (grep):
 * - Crear pipe2
 * - Fork hijo2
 * - Hijo2: dup2(prev_fd, STDIN), dup2(pipe2[1], STDOUT), exec grep
 * - Padre: close(prev_fd), close(pipe2[1]), prev_fd = pipe2[0]
 *
 * Iteración 3 (wc):
 * - No crear pipe (último comando)
 * - Fork hijo3
 * - Hijo3: dup2(prev_fd, STDIN), exec wc
 * - Padre: close(prev_fd), wait_all()
 */

/*
 * ERRORES COMUNES Y CÓMO EVITARLOS:
 *
 * 1. DEADLOCK POR DESCRIPTORES ABIERTOS:
 *    - Si no cerramos write end en el padre, grep nunca recibe EOF
 *    - Resultado: el pipeline se cuelga indefinidamente
 *
 * 2. BROKEN PIPE:
 *    - Si cerramos descriptores en orden incorrecto
 *    - El proceso puede recibir SIGPIPE y terminar
 *
 * 3. ZOMBIE PROCESSES:
 *    - Si no hacemos wait() de todos los hijos
 *    - Los procesos quedan como zombies en el sistema
 *
 * 4. LEAKS DE DESCRIPTORES:
 *    - No cerrar descriptores que no se usan
 *    - Puede agotar la tabla de descriptores del sistema
 */

/*
 * PUNTOS CLAVE PARA EL EXAMEN:
 *
 * 1. ORDEN DE OPERACIONES:
 *    - Crear pipe ANTES de fork
 *    - Configurar redirecciones en el hijo
 *    - Cerrar descriptores apropiados en padre e hijo
 *
 * 2. GESTIÓN DE PREV_FD:
 *    - Representa la "salida" del comando anterior
 *    - Se convierte en la "entrada" del comando actual
 *    - Debe cerrarse después de usar
 *
 * 3. ÚLTIMO COMANDO ESPECIAL:
 *    - No necesita pipe de salida
 *    - Su stdout va al terminal
 *    - Verificar cmds[i + 1] antes de crear pipe
 *
 * 4. MANEJO DE ERRORES:
 *    - Verificar retorno de pipe(), fork(), dup2()
 *    - Cerrar descriptores en caso de error
 *    - Retornar 1 si algún comando falla
 *
 * 5. SINCRONIZACIÓN:
 *    - wait() en bucle hasta que no haya más hijos
 *    - Verificar códigos de salida de todos los procesos
 *    - Un solo proceso fallido hace fallar toda la pipeline
 */
