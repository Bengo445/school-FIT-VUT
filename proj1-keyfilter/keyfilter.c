#include <stdio.h>
#include <ctype.h>
#include <string.h>


// Exhaust current adress (read stdin) until new line or EOF
int exhaust_current_adress(void) { 
    //printf("-Exhausting current adress"); //DEBUG
    int nxt;
    while ((nxt = getchar()) != EOF){
        if (nxt == '\n'){
            break;
        }
    }
    //printf("-Finished\n"); //DEBUG
    return 0;
}

// MAIN FUNC
int main(int argc, char *argv[]) {

    // Missing prefix input (argument 2), treat as empty string
    if (argc == 1) {
        argv[1] = "";
    }

    int input_len = strlen(argv[1]); //Input (adress prefix) length

    // Support for adresseses up to 100 chars long
    char matching_adress[101] = ""; // space to save a fully matching adress
    int matching_adress_found = 0; // for simpler output checks
    char partial_adress[101] = ""; // apace to save a partially matching adress in case of being the only possible matching adress
    char current_adress[101] = ""; // space for adress that is currently being read
    int current_adress_len = 0;

    //The possible_keys array serves as a "punchcard", a way to save keys which are concidered "Enabled" for the output.
    //It is naturally sorted and doesn't have duplicates due to the way it's used.
    //Every index is 0 by default and corresponds to a visible ASCII symbol -> ASCII symbols from code 32(<space>) to code 126(~).
    //Therefore the array size is 126-32 -> 94.
    //For better effectivity, we could exclude the range of lower characters, however that would make it more complex.
    //-(example: if we find that 'A' should be "Enabled", we write a 1 to possible_keys['A'-32] -> possible_keys[32])
    char possible_keys[94];
    for(int i = 0; i<94; i++){possible_keys[i] = 0;}

    int possible_keys_count = 0;

    int next_key; // currently reading key in adress 
    int check_index = 0;


    //printf("- Input string: %s\n", argv[1]); //DEBUG
    
    // MAIN LOOP
    // We stream-read the adresses (character by character) until end of file or until artificial adress-length limit is reached.
    while (((next_key = getchar()) != EOF) && (current_adress_len < 100)) {

        // If we've reached the end of current adress, and our prefix input matches it fully, save it as a matching adress
        if (next_key == '\n' && input_len == current_adress_len) {

            // Count as a matching adress (for output checks)
            matching_adress_found = 1;

            // Copy current_adress buffer into matching_adress
            strcpy(matching_adress, current_adress);

            // Moving onto next adress next loop
            current_adress[0] = '\0';
            current_adress_len = 0;
            check_index = 0;
            //printf("(EOL)\n"); //DEBUG
            continue;
        }

        // Extend current_adress by the currently read next_key, then terminate the string
        current_adress[current_adress_len++] = next_key;
        current_adress[current_adress_len] = '\0';

        //if input prefix ran out of chars, save the next key in adress to possible keys.
        //then, save the whole adress to cmpd_adress for later use in case we overall find only one adress that fits with the input prefix, but doesnt fully match
        if (check_index >= input_len) { 

            //save possible key only if its in the correct ascii code range
            if (next_key >= 32 && next_key <= 126) {
                possible_keys[toupper(next_key)-32] = 1;
                possible_keys_count++;
            }

            //printf("-Exhausting current adress into current_adress"); //DEBUG
            // Read and write the rest of the current adress into the current_adress buffer
            int nxt;
            while (((nxt = getchar()) != EOF) && (current_adress_len < 100)) {
                if (nxt == '\n') {break;}
                current_adress[current_adress_len++] = nxt;
                current_adress[current_adress_len] = '\0';
            }
            //printf("-Finished\n"); //DEBUG
            
            // Empty current_adress buffer into partial_adress
            strcpy(partial_adress, current_adress);

            // Moving onto next adress next loop
            current_adress[0] = '\0';
            current_adress_len = 0;
            check_index = 0;
            continue;
        }

        // next_key matches with input prefix so far, continue to check next key in adress
        if (toupper(next_key) == toupper(argv[1][check_index])) {
            check_index++;
            continue;

        } else { // next_key doesn't match input prefix anymore, ignore adress
            exhaust_current_adress();

            // Moving onto next adress next loop
            current_adress[0] = '\0';
            current_adress_len = 0;
            check_index = 0;
            continue;
        }

    }

    // END OF FILE "adresy.txt"
    //printf("(EOF)\n");
    
    // OUTPUT
    // If at least one fully matching, non-empty adress was found, print to output
    //printf("matched: %i\n", matching_adresses);
    if ((matching_adress_found >= 1) && (strlen(matching_adress) > 0)) {
        printf("Found: ");
        int i = 0;
        while (matching_adress[i] != '\0') {
            printf("%c", toupper(matching_adress[i++]));
        }
        printf("\n");
    }

    // If at least one possible key was found
    if (possible_keys_count > 0) {
        
        // If exactly one possible key found, concider the last adress matched with it as Found
        if (possible_keys_count == 1 && matching_adress_found == 0) {
            printf("Found: ");
            int i = 0;
            while (partial_adress[i] != '\0') {
                printf("%c", toupper(partial_adress[i++]));
            }
            printf("\n");
            return 0;
        }
        
        // Print out every character in possible_keys that has a 1 at it's index
        printf("Enable: ");
        for(int i = 0; i<=94; i++) {
            if (possible_keys[i] == 1) {
                printf("%c", i+32); //i+32 due to possible_keys array setup
            }
        }
        printf("\n");
        return 0;

    // no possible keys found or adress matching prefix found
    } else if (matching_adress_found == 0) { 
        printf("Not found\n");
    }

    return 0;
}