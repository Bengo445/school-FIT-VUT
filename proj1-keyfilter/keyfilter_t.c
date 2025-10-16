#include <stdio.h>
#include <ctype.h>
#include <string.h>


// When done with an adress, read until newline or EOF.
int exhaust_current_adress(void){ 
    //printf("-Exhausting current adress");
    int nxt;
    while ((nxt = getchar()) != EOF){
        if (nxt == '\n'){
            break;
        }
    }
    //printf("-Finished\n");
    return 0;
}

// MAIN FUNC
// argv[1] - adress search prefix (string)
// argv[2] - debug mode (int (bool))
int main(int argc, char *argv[]) {

    // if main input argument argv[1] (adress prefix) is missing, make it an empty string
    if (argc == 1) { 
        argv[1] = "";
    }

    int input_len = strlen(argv[1]);

    // matched_adress -> saved adress that is a 100% match
    char matched_adress[102] = ""; // 100len + 1 \0
    char cmpd_adress[102] = "";
    int matching_adresses = 0;
    char current_adress[102] = "";
    int current_adress_len = 0;

    //Prazdna array ktoru naplnime jednotkami, az na poslednu hodnotu (lebo 0 terminuje string)
    //pointa je mat miesto na kazdy viditelný ASCII znak zaroven (ASCII od 32(<space>) do 126(~))
    //ked budeme zapisovat "enabled" znaky pocas programu, staci prepisat 1 v tomto liste na hodnotu charakteru
    //napr. chcem napisat 'A' tak napisem to na poziciu 'A'-32 v array
    //vysledok nam umozni nakonci programu mať alfabeticky zoradeni zoznam "enabled" znakov
    //(pre lepsiu efektivitu vieme odstranit malé písmená, problém nastáva ked si vsimneme ze za nimi su este viditelne znaky v ASCII tabulke)
    char possible_keys[95]; 
    for(int i = 0; i<94; i++){possible_keys[i] = 1;}
    possible_keys[94] = '\0';

    int has_possible_key = 0;
    int next_key;
    int check_index = 0;

    //printf("- Input string: %s\n", argv[1]);
    
    // MAIN LOOP
    int safecount = 0;
    while (((next_key = getchar()) != EOF) && (current_adress_len < 100)){

        if (next_key == '\n'){ // koniec adresy, prechod na novú adresu

            strcpy(matched_adress, current_adress); // prepis terajsiu adresu do momentalnej matched adresy
            matching_adresses++;
            current_adress[0] = '\0';
            current_adress_len = 0;

            check_index = 0;
            //printf("(EOL)\n"); 
            continue;
        }

        // save current adress throughout input stream
        //printf("CURLEN:%i ", current_adress_len);
        current_adress[current_adress_len++] = next_key;
        current_adress[current_adress_len] = '\0';

        //printf("%c", next_key);

        if (check_index >= input_len){ //input ran out of chars, save the next key in adress to possible keys, begin next adress
            //printf("%i", next_key);
            possible_keys[next_key-32] = next_key;

            /*
            for(int i = 0; i < 105; i++){
                printf("%i ", possible_keys[i]);
            }
            */
            //printf("\n");
            has_possible_key++;
            check_index = 0;

            //printf("-Exhausting current adress into current_adress");
            int nxt;
            int safecount_2 = 0;
            while (((nxt = getchar()) != EOF) && (current_adress_len < 100)){
                if (nxt == '\n'){
                    break;
                }
                current_adress[current_adress_len++] = nxt;
                current_adress[current_adress_len] = '\0';
            }
            //printf("-Finished\n");
            
            strcpy(cmpd_adress, current_adress); // prepis terajsiu adresu do momentalnej matched adresy

            continue;
        }

        if (toupper(next_key) == toupper(argv[1][check_index])){ //next_key matches input so far, continue checking
            check_index++;
            continue;
        } else { //next_key doesn't match input anymore, skip adress
            current_adress[0] = '\0';
            current_adress_len = 0;
            check_index = 0;
            exhaust_current_adress();
            continue;
        }

    }

    // END OF FILE "adresy.txt"
    //printf("(EOF)\n");
    
    // OUTPUT
    //matched string
    //printf("matched: %i\n", matching_adresses);
    if ((matching_adresses >= 1) && (strlen(matched_adress) > 0)){
        printf("Found: ");
        int i = 0;
        while (matched_adress[i] != '\0'){
            printf("%c", toupper(matched_adress[i++]));
        }
        printf("\n");
    }
    //enable chars (!SORT)
    if (has_possible_key){
        
        if (has_possible_key == 1 && matching_adresses == 0){
            printf("Found: ");
            int i = 0;
            while (cmpd_adress[i] != '\0'){
                printf("%c", toupper(cmpd_adress[i++]));
            }
            printf("\n");
            return 0;
        }
        
        printf("Enable: ");
        for(int i = 0; i<94; i++){
            if (possible_keys[i] != 1){
                printf("%c", toupper(possible_keys[i]));
            }
        }
        printf("\n");
        return 0;
    } else if (matching_adresses != 1) { // no chars saved, adress with prefix not found
        printf("Not found\n");
    }

    return 0;
}
