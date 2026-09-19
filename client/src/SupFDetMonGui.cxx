#include "TSupFDetMonGui.h"

#include <TApplication.h>
#include <TGClient.h>

int main(int argc, char** argv)
{
    TApplication application("SupFDetMonGui", &argc, argv);

    new TSupFDetMonGui(gClient->GetRoot(), 1200, 700);

    application.Run();
    return 0;
}
