/* Authored module, deliberately independent of Android/game code. */
extern float sf_fixture_add(float,float);
static int initial=7;
static int *volatile relocated=&initial;
static int counter;
__attribute__((visibility("default"))) float sf_fixture_entry(float a,float b){
    counter++;return sf_fixture_add(a,b)+(float)(*relocated+counter);
}
