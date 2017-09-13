#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

/*****************************************************************************
 *                                animation
 *****************************************************************************/
/**
 * An animation 
 *
 *****************************************************************************/
PUBLIC void animation()
{

int i = 0;
clear();
printf("                                                                  .. \n");
printf("                                                                   ++\n");
printf("                                                                    :\n");
printf("                                                                     \n");
printf("                                                                     \n");
printf("                                                                     \n");
printf("                                                                     \n");
printf("                                                                `-:/+\n");
printf("                                                             `-+oo+os\n");
printf("                                                             ```   .+\n");
printf("                                                                 `omM\n");
printf("                                                             .:oymMMM\n");
printf("                                                           -oosdMMMMM\n");
printf("                                                          -:`  dMMMMM\n");
printf("                                                          `   -MMMMMM\n");
printf("                                                             `+MMMMMM\n");
printf("                                                            .yNMMMMMM\n");
printf("                                                           y+-mMMMMMM\n");
printf("                                                           :-  :MMMMM\n");
printf("                                                           `   `hMMMM\n");
printf("                                                                `-MMM\n");
printf("                                                             `-+oo+os\n");
printf("                                                             ```   .+\n");
printf("                                                                 `omM\n");
printf("                                                             .:oymMMM\n");
printf("                                                         -oosdMMMMM\n\n");
milli_delay(1500);


clear();
printf("                                                              ..     \n");
printf("                                                               ++`   \n");
printf("                                                                :ho. \n");
printf("                                                                 `/hh\n");
printf("                                                                   `-\n");
printf("                                                                     \n");
printf("                                                                     \n");
printf("                                                            `-:/+++/:\n");
printf("                                                         `-+oo+osdMMM\n");
printf("                                                         ```   .+mMMM\n");
printf("                                                             `omMMMMM\n");
printf("                                                         .:oymMMMMMMM\n");
printf("                                                       -oosdMMMMMMMMM\n");
printf("                                                      -:`  dMMMMMMMMM\n");
printf("                                                      `   -MMMMMMMMMM\n");
printf("                                                         `+MMMMMMMMMM\n");
printf("                                                        .yNMMMMMMMMMM\n");
printf("                                                       y+-mMMMMMMMMMM\n");
printf("                                                       :-  :MMMMMMMMM\n");
printf("                                                       `   `hMMMMMMMM\n");
printf("                                                            `-MMMMMMN\n");
printf("                                                         `-+oo+osdMMM\n");
printf("                                                         ```   .+mMMM\n");
printf("                                                             `omMMMMM\n");
printf("                                                         .:oymMMMMMMM\n");
printf("                                                     -oosdMMMMMMMMM\n\n");
milli_delay(3000);


clear();
printf("                                                        ..           \n");
printf("                                                         ++`         \n");
printf("                                                          :ho.       \n");
printf("                                                           `/hh+.    \n");
printf("                                                             `-odds/-\n");
printf("                                                                `.+yd\n");
printf("                                                                   `:\n");
printf("                                                      `-:/+++/:-:ohmN\n");
printf("                                                   `-+oo+osdMMMNMMMMM\n");
printf("                                                   ```   .+mMMMMMMMMM\n");
printf("                                                       `omMMMMMMMMMMM\n");
printf("                                                   .:oymMMMMMMMMMMMMM\n");
printf("                                                 -oosdMMMMMMMMMMMMMMd\n");
printf("                                                -:`  dMMMMMMMMMMMMMd.\n");
printf("                                                `   -MMMMMMMMMMMMMM- \n");
printf("                                                   `+MMMMMMMMMMMMMM- \n");
printf("                                                  .yNMMMMMMMMMMMMMMd.\n");
printf("                                                 y+-mMMMMMMMMMMMMMMm/\n");
printf("                                                 :-  :MMMMMMMMMMMMMMM\n");
printf("                                                 `   `hMMMMMMMMMMMMMM\n");
printf("                                                      `-MMMMMMNMMMMMM\n");
printf("                                                   `-+oo+osdMMMNMMMMM\n");
printf("                                                   ```   .+mMMMMMMMMM\n");
printf("                                                       `omMMMMMMMMMMM\n");
printf("                                                   .:oymMMMMMMMMMMMMM\n");
printf("                                               -oosdMMMMMMMMMMMMMMd\n\n");
milli_delay(3000);

clear();
printf("                                                ..                   \n");
printf("                                                 ++`                 \n");
printf("                                                  :ho.        `.-/++/\n");
printf("                                                   `/hh+.         ``:\n");
printf("                                                     `-odds/-`       \n");
printf("                                                        `.+ydmdyo/:--\n");
printf("                                                           `:+hMMMNNN\n");
printf("                                              `-:/+++/:-:ohmNMMMMMMMM\n");
printf("                                           `-+oo+osdMMMNMMMMMMMMMMMMM\n");
printf("                                           ```   .+mMMMMMMMMMMMMMMMMM\n");
printf("                                               `omMMMMMMMMMMMMMMMMMMN\n");
printf("                                           .:oymMMMMMMMMMMMMMNdo/hMMd\n");
printf("                                         -oosdMMMMMMMMMMMMMMd:`  `yMM\n");
printf("                                        -:`  dMMMMMMMMMMMMMd.     `mM\n");
printf("                                        `   -MMMMMMMMMMMMMM-       -m\n");
printf("                                           `+MMMMMMMMMMMMMM-        .\n");
printf("                                          .yNMMMMMMMMMMMMMMd.        \n");
printf("                                         y+-mMMMMMMMMMMMMMMm/`       \n");
printf("                                         :-  :MMMMMMMMMMMMMMMMmy/.`  \n");
printf("                                         `   `hMMMMMMMMMMMMMMMMMMNds/\n");
printf("                                              `-MMMMMMNMMMMMMMMMMMm+-\n");
printf("                                           `-+oo+osdMMMNMMMMMMMMMMMMM\n");
printf("                                           ```   .+mMMMMMMMMMMMMMMMMM\n");
printf("                                               `omMMMMMMMMMMMMMMMMMMN\n");
printf("                                           .:oymMMMMMMMMMMMMMNdo/hMMd\n");
printf("                                       -oosdMMMMMMMMMMMMMMd:`  `yMM\n\n");
milli_delay(3000);


clear();
printf("                                           ..                        \n");
printf("                                            ++`                      \n");
printf("                                             :ho.        `.-/++/.    \n");
printf("                                              `/hh+.         ``:sds: \n");
printf("                                                `-odds/-`        .MNd\n");
printf("                                                   `.+ydmdyo/:--/yMMM\n");
printf("                                                      `:+hMMMNNNMMMdd\n");
printf("                                         `-:/+++/:-:ohmNMMMMMMMMMMMm+\n");
printf("                                      `-+oo+osdMMMNMMMMMMMMMMMMMMMMMM\n");
printf("                                      ```   .+mMMMMMMMMMMMMMMMMMMMMMM\n");
printf("                                          `omMMMMMMMMMMMMMMMMMMNMdydM\n");
printf("                                      .:oymMMMMMMMMMMMMMNdo/hMMd+ds-:\n");
printf("                                    -oosdMMMMMMMMMMMMMMd:`  `yMM+.+h+\n");
printf("                                   -:`  dMMMMMMMMMMMMMd.     `mMNo..+\n");
printf("                                   `   -MMMMMMMMMMMMMM-       -mMMmo-\n");
printf("                                      `+MMMMMMMMMMMMMM-        .smMy:\n");
printf("                                     .yNMMMMMMMMMMMMMMd.         .+dm\n");
printf("                                    y+-mMMMMMMMMMMMMMMm/`          ./\n");
printf("                                    :-  :MMMMMMMMMMMMMMMMmy/.`       \n");
printf("                                    `   `hMMMMMMMMMMMMMMMMMMNds/.`   \n");
printf("                                         `-MMMMMMNMMMMMMMMMMMm+-+mMNd\n");
printf("                                      `-+oo+osdMMMNMMMMMMMMMMMMMMMMMM\n");
printf("                                      ```   .+mMMMMMMMMMMMMMMMMMMMMMM\n");
printf("                                          `omMMMMMMMMMMMMMMMMMMNMdydM\n");
printf("                                      .:oymMMMMMMMMMMMMMNdo/hMMd+ds-:\n");
printf("                                  -oosdMMMMMMMMMMMMMMd:`  `yMM+.+h+\n\n");
milli_delay(3000);


clear();
printf("                                   ..                                \n");
printf("                                    ++`                              \n");
printf("                                     :ho.        `.-/++/.            \n");
printf("                                      `/hh+.         ``:sds:         \n");
printf("                                        `-odds/-`        .MNd/`      \n");
printf("                                           `.+ydmdyo/:--/yMMMMd/     \n");
printf("                                              `:+hMMMNNNMMMddNMMh:`  \n");
printf("                                 `-:/+++/:-:ohmNMMMMMMMMMMMm+-+mMNd` \n");
printf("                              `-+oo+osdMMMNMMMMMMMMMMMMMMMMMMNmNMMM/`\n");
printf("                              ```   .+mMMMMMMMMMMMMMMMMMMMMMMMMMMMMNm\n");
printf("                                  `omMMMMMMMMMMMMMMMMMMNMdydMMdNMMMMM\n");
printf("                              .:oymMMMMMMMMMMMMMNdo/hMMd+ds-:h/-yMdyd\n");
printf("                            -oosdMMMMMMMMMMMMMMd:`  `yMM+.+h+.-  /y `\n");
printf("                           -:`  dMMMMMMMMMMMMMd.     `mMNo..+y/`  .  \n");
printf("                           `   -MMMMMMMMMMMMMM-       -mMMmo-./s/.`  \n");
printf("                              `+MMMMMMMMMMMMMM-        .smMy:.``-+oo+\n");
printf("                             .yNMMMMMMMMMMMMMMd.         .+dmh+:.  `-\n");
printf("                            y+-mMMMMMMMMMMMMMMm/`          ./o+-`    \n");
printf("                            :-  :MMMMMMMMMMMMMMMMmy/.`               \n");
printf("                            `   `hMMMMMMMMMMMMMMMMMMNds/.`           \n");
printf("                                 `-MMMMMMNMMMMMMMMMMMm+-+mMNd`       \n");
printf("                              `-+oo+osdMMMNMMMMMMMMMMMMMMMMMMNmNMMM/`\n");
printf("                              ```   .+mMMMMMMMMMMMMMMMMMMMMMMMMMMMMNm\n");
printf("                                  `omMMMMMMMMMMMMMMMMMMNMdydMMdNMMMMM\n");
printf("                              .:oymMMMMMMMMMMMMMNdo/hMMd+ds-:h/-yMdyd\n");
printf("                          -oosdMMMMMMMMMMMMMMd:`  `yMM+.+h+.-  /y `\n\n");
milli_delay(3000);

clear();
printf("                            ..                                       \n");
printf("                             ++`                                     \n");
printf("                              :ho.        `.-/++/.                   \n");
printf("                               `/hh+.         ``:sds:                \n");
printf("                                 `-odds/-`        .MNd/`             \n");
printf("                                    `.+ydmdyo/:--/yMMMMd/            \n");
printf("                                       `:+hMMMNNNMMMddNMMh:`         \n");
printf("                          `-:/+++/:-:ohmNMMMMMMMMMMMm+-+mMNd`        \n");
printf("                       `-+oo+osdMMMNMMMMMMMMMMMMMMMMMMNmNMMM/`       \n");
printf("                       ```   .+mMMMMMMMMMMMMMMMMMMMMMMMMMMMMNmho:.`  \n");
printf("                           `omMMMMMMMMMMMMMMMMMMNMdydMMdNMMMMMMMMdo+-\n");
printf("                       .:oymMMMMMMMMMMMMMNdo/hMMd+ds-:h/-yMdydMNdNdNN\n");
printf("                     -oosdMMMMMMMMMMMMMMd:`  `yMM+.+h+.-  /y `/m.:mmm\n");
printf("                    -:`  dMMMMMMMMMMMMMd.     `mMNo..+y/`  .   .  -/.\n");
printf("                    `   -MMMMMMMMMMMMMM-       -mMMmo-./s/.`         \n");
printf("                       `+MMMMMMMMMMMMMM-        .smMy:.``-+oo+//:-.` \n");
printf("                      .yNMMMMMMMMMMMMMMd.         .+dmh+:.  `-::/+:. \n");
printf("                     y+-mMMMMMMMMMMMMMMm/`          ./o+-`       .   \n");
printf("                     :-  :MMMMMMMMMMMMMMMMmy/.`                      \n");
printf("                     `   `hMMMMMMMMMMMMMMMMMMNds/.`                  \n");
printf("                          `-MMMMMMNMMMMMMMMMMMm+-+mMNd`              \n");
printf("                       `-+oo+osdMMMNMMMMMMMMMMMMMMMMMMNmNMMM/`       \n");
printf("                       ```   .+mMMMMMMMMMMMMMMMMMMMMMMMMMMMMNmho:.`  \n");
printf("                           `omMMMMMMMMMMMMMMMMMMNMdydMMdNMMMMMMMMdo+-\n");
printf("                       .:oymMMMMMMMMMMMMMNdo/hMMd+ds-:h/-yMdydMNdNdNN\n");
printf("                   -oosdMMMMMMMMMMMMMMd:`  `yMM+.+h+.-  /y `/m.:mmm\n\n");
milli_delay(3000);

clear();
printf("                     ..                                              \n");
printf("                      ++`                                            \n");
printf("                       :ho.        `.-/++/.                          \n");
printf("                        `/hh+.         ``:sds:                       \n");
printf("                          `-odds/-`        .MNd/`                    \n");
printf("                             `.+ydmdyo/:--/yMMMMd/                   \n");
printf("                                `:+hMMMNNNMMMddNMMh:`                \n");
printf("                   `-:/+++/:-:ohmNMMMMMMMMMMMm+-+mMNd`               \n");
printf("                `-+oo+osdMMMNMMMMMMMMMMMMMMMMMMNmNMMM/`              \n");
printf("                ```   .+mMMMMMMMMMMMMMMMMMMMMMMMMMMMMNmho:.`         \n");
printf("                    `omMMMMMMMMMMMMMMMMMMNMdydMMdNMMMMMMMMdo+-       \n");
printf("                .:oymMMMMMMMMMMMMMNdo/hMMd+ds-:h/-yMdydMNdNdNN+      \n");
printf("              -oosdMMMMMMMMMMMMMMd:`  `yMM+.+h+.-  /y `/m.:mmmN      \n");
printf("             -:`  dMMMMMMMMMMMMMd.     `mMNo..+y/`  .   .  -/.s      \n");
printf("             `   -MMMMMMMMMMMMMM-       -mMMmo-./s/.`         `      \n");
printf("                `+MMMMMMMMMMMMMM-        .smMy:.``-+oo+//:-.`        \n");
printf("               .yNMMMMMMMMMMMMMMd.         .+dmh+:.  `-::/+:.        \n");
printf("              y+-mMMMMMMMMMMMMMMm/`          ./o+-`       .          \n");
printf("              :-  :MMMMMMMMMMMMMMMMmy/.`                             \n");
printf("              `   `hMMMMMMMMMMMMMMMMMMNds/.`                         \n");
printf("                   `-MMMMMMNMMMMMMMMMMMm+-+mMNd`                     \n");
printf("                `-+oo+osdMMMNMMMMMMMMMMMMMMMMMMNmNMMM/`              \n");
printf("                ```   .+mMMMMMMMMMMMMMMMMMMMMMMMMMMMMNmho:.`         \n");
printf("                    `omMMMMMMMMMMMMMMMMMMNMdydMMdNMMMMMMMMdo+-       \n");
printf("                .:oymMMMMMMMMMMMMMNdo/hMMd+ds-:h/-yMdydMNdNdNN+      \n");
printf("              -oosdMMMMMMMMMMMMMMd:`  `yMM+.+h+.-  /y `/m.:mmmN     \n\n");
milli_delay(3000);


}
