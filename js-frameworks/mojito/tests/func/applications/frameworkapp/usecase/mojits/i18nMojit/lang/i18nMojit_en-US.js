YUI.add("lang/i18nMojit_en-US", function(Y) {

    Y.Intl.add(

        "i18nMojit",   // associated module
        "en-US",    // BCP 47 language tag

        // key-value pairs for this module and language
        {
            TITLE: "Enjoy your Flickr Images!",
            chosecookie: "First Choice {name}",
            chosefruit: "Second Choice {0}",
            chosecake: "Third Choice {0} and {1}"
        }
    );
}, "3.1.0", {requires: ['intl']});