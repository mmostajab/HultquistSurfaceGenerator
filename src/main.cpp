#include "application.h"

Application app;

int main() {

    app.init(800, 600);

    app.run();
    
    app.shutdown();

    return 0;
} 