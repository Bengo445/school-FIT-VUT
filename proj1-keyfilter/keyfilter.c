#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>


// Length in characters for which this program supports reading and saving adresses.
// In case adress longer than supported length, it is ignored/skipped in the list of adresses.
// A possible key may be saved from an adress that is too long
// Must not be negative.
const int MAX_SUPPORTED_ADRESS_LEN = 100;

// Exhaust current adress (read stdin) until new line character or EOF.
void exhaust_current_adress(void) { 
    int nxt;
    while ((nxt = getchar()) != EOF){
        if (nxt == '\n') {break;}
    }
}

// MAIN FUNCTION
// argv[1] = adress search prefix input
int main(int argc, char *argv[]) {
    // Missing prefix input (argument 2), treat as empty string
    if (argc == 1) {argv[1] = "";}
    //printf("- Input string: %s\n", argv[1]); //DEBUG
    int input_len = strlen(argv[1]);

    // Support for adresseses up to MAX_SUPPORTED_ADRESS_LEN chars long
    if (MAX_SUPPORTED_ADRESS_LEN < 0){
        printf("Invalid MAX_SUPPORTED_ADRESS_LEN, must not be negative.");
        return 1;
    }

    char matching_adress[MAX_SUPPORTED_ADRESS_LEN+1]; matching_adress[0] = '\0'; // Space to save a fully matching adress
    bool matching_adress_found = false; // To simplify output checks
    char partial_adress[MAX_SUPPORTED_ADRESS_LEN+1]; partial_adress[0] = '\0'; // Space to save a partially matching adress in case of being the only logically possible matching adress
    char current_adress[MAX_SUPPORTED_ADRESS_LEN+1]; current_adress[0] = '\0'; // "Buffer" space for adress that is currently being read
    int current_adress_len = 0;

    int next_key = 0; // Currently reading key in adress 
    int check_index = 0; // Index for comparing the next key and input prefix while reading an adress
    bool carriage_return = false; // When set to 1, next main loop will behave as if reading a new adress

    //The possible_keys array serves as a way to save keys which are concidered "Enabled" for the output.
    //Due to the way we use it, it is naturally sorted and doesn't have duplicates.
    //Each index has a value of 0 by default and corresponds to a visible ASCII symbol -> ASCII symbols from code 32(<space>) to code 126(~).
    //Therefore the array size is 126-32 = 94.
    //For better effectivity, we could exclude the range of lower characters (we dont use it), however that would make it more complex.
    //-(example: if we find that 'A' should be "Enabled", we write a 1 to possible_keys['A'-32] -> possible_keys[32])
    char possible_keys[94];
    for(int i = 0; i<94; i++){possible_keys[i] = 0;}
    int possible_keys_count = 0;
    

    // MAIN LOOP
    // We stream-read the adresses (character by character) until end of file
    while (next_key != EOF) {
        next_key = getchar(); //outside of while expression due to issues with reading an adress without a \n at the end

        // Every time a new main loop would start reading a new adress, carriage_return should be 1
        // Clear current_adress buffer and reset check_index
        if (carriage_return){
            carriage_return = false;
            current_adress[0] = '\0';
            current_adress_len = 0;
            check_index = 0;
        }

        // If harcoded adress-length limit is reached or non-ascii character found, ignore adress
        if (current_adress_len >= MAX_SUPPORTED_ADRESS_LEN || next_key > 127){
            exhaust_current_adress();

            // Moving onto next adress in next loop
            carriage_return = true;
            continue;
        }

        // If we've reached the end of current adress,
        if (next_key == '\n' || next_key == EOF){

            /*if (input_len < check_index){
                exhaust_current_adress();
            }*/

            //If our search prefix input matches it exactly, save it as a matching adress
            if(input_len == current_adress_len && current_adress_len > 0) {

                matching_adress_found = true;

                // Copy current_adress buffer into matching_adress
                strcpy(matching_adress, current_adress);

                // Moving onto next adress in next loop
                carriage_return = true;
                continue;
            }
        }

        // Update current_adress with the currently read next_key, then terminate the string
        current_adress[current_adress_len++] = next_key;
        current_adress[current_adress_len] = '\0';

        // If input prefix ran out of chars, save the next key in current adress stream to possible keys.
        // Then, save the whole adress to partial_adress for later use in case we overall find only one adress that fits with the input prefix, but doesnt fully match
        if (input_len <= check_index) { 

            // Save possible key only if its in the correct ascii code range
            if (next_key >= 32 && next_key <= 126) {
                possible_keys[toupper(next_key)-32] = 1;
                possible_keys_count++;
            }

            // Read and write the rest of the current adress into the current_adress buffer
            int nxt;
            while (((nxt = getchar()) != EOF) && (current_adress_len < MAX_SUPPORTED_ADRESS_LEN)) {
                if (nxt == '\n') {break;}
                current_adress[current_adress_len++] = nxt;
                current_adress[current_adress_len] = '\0';
            }
            
            // Empty current_adress buffer into partial_adress
            strcpy(partial_adress, current_adress);

            // Moving onto next adress in next loop
            carriage_return = true;
            continue;
        }

        // next_key matches with input prefix so far
        if (toupper(next_key) == toupper(argv[1][check_index])) {
            // Continue to check next key in current adress
            check_index++;

        // next_key doesn't match input prefix anymore, ignore adress
        } else { 
            // Moving onto next adress in next loop
            carriage_return = true;
        }
    }
    // FINISHED READING FILE "adresy.txt"
    // MAIN LOOP END
    
    // OUTPUT - Output matching adress first, then enabled characters. Else "Not found".
    // If (at least) 1 fully matching, non-empty adress was found, print matching to output as "Found"
    if ((matching_adress_found) && (strlen(matching_adress) > 0)) {
        printf("Found: ");
        int i = 0;
        while (matching_adress[i] != '\0') {
            printf("%c", toupper(matching_adress[i++])); //(convert to uppercase)
        }
        printf("\n");
    }
    // Cases where at least 1 possible key was found
    if (possible_keys_count > 0) {

        // Exactly 1 possible key found, concider the last adress that was partially-matched as "Found"
        if (possible_keys_count == 1 && !matching_adress_found) {
            printf("Found: ");
            int i = 0;
            while (partial_adress[i] != '\0') {
                printf("%c", toupper(partial_adress[i++])); //(convert to uppercase)
            }
            printf("\n");

        // More than 1 possible keys found, output every character in possible_keys that has a 1 at it's index
        } else {
            printf("Enable: ");
            for(int i = 0; i<=94; i++) {
                if (possible_keys[i] == 1) {
                    printf("%c", i+32); //i+32 due to possible_keys array setup
                }
            }
            printf("\n");
        }

    // No possible keys or single partially matching adress found
    } else if (!matching_adress_found) { 
        printf("Not found\n");
    }

    return 0;
}