           ~WEMOS~                           ~ProMini~
        #############                    #################
        # D5     D1 #                    # 10           #
        # D6     D2 #                    # 11           #
        # D7     D3 #                    # 12
        # D8     D4 #                    # 13         5 #   
        #           #                    #            4 #
        #############                    #            3 #
                                         #            2 #
                                         #              #
                                         ################
~ WEMOS SIDE
PIN |TYPE   |NAME       |TO
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
D1  |OUTPUT |ESP_UP     | 11
D2  |OUTPUT |ESP_DOWN   | 12
D5  |INPUT  |REL_UP_IND | 13
D6  |INPUT  |REL_DN_IND | 10

~ProMini SIDE
PIN |TYPE   | NAME      |TO
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
5   |INPUT  |SW_DOWN    |Switch hardware DOWN
4   |INPUT  |SW_UP      |Switch hardware UP
3   |OUTPUT |RELAY_UP   |Relay Module UP
2   |OUTPUT |RELAY_DOWN |Relay Module DOWN
