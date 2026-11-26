/* Authored module, deliberately independent of Android/game code. */
extern float sf_fixture_add(float,float);
extern void sf_fixture_lifecycle_event(int);
static int initial;
__attribute__((constructor)) static void initialize(void){initial=7;sf_fixture_lifecycle_event(1);}
__attribute__((destructor)) static void finalize(void){initial=0;sf_fixture_lifecycle_event(2);}
static int *volatile relocated=&initial;
static int counter;
__attribute__((visibility("default"))) float sf_fixture_entry(float a,float b){
    counter++;return sf_fixture_add(a,b)+(float)(*relocated+counter);
}
