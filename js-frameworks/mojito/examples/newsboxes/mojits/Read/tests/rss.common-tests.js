/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon: true */
/*global YUI,YUITest*/


YUI.add('read-model-rss-tests', function(Y, NAME) {
    'use strict';

    var suite = new YUITest.TestSuite(NAME),
        A = YUITest.Assert,
        rss;

    function mockresp() {
        return {
            'query': {
                'count': 20,
                'created': '2011-10-04T07: 03: 53Z',
                'lang': 'en-US',
                'results': {
                    'item': [
                        {
                            'title': 'YouTube\u2019s Rick Klau Joins Google Ventures To Head Startup University',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/HsSbbXruHiM\/',
                            'pubDate': 'Tue, 04 Oct 2011 05: 23: 25 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/screen-shot-2011-10-03-at-8-06-59-pm1.png?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="Screen Shot 2011-10-03 at 8.06.59 PM" title="Screen Shot 2011-10-03 at 8.06.59 PM" style="float: left; margin: 0 10px 7px 0;" \/>Everybody seems to be bulking up <a href="http: \/\/techcrunch.com\/2011\/10\/03\/mg-siegler-will-become-our-apple-columnist-and-join-crunchfund-as-a-vc\/">on VCs today<\/a>(!); <a href="http: \/\/www.googleventures.com">Google Ventures<\/a> is also adding to its roster, bring YouTube Product Manager <a href="http: \/\/www.crunchbase.com\/person\/rick-klau">Rick Klau <\/a>to the team as Partner, <b>Startup University</b>.\n\nStartup University is Google Ventures\' efforts to transfer as much knowledge and as much background information as possible to its portfolio companies.\n\nIt does this by providing <span>\n\neducational opportunities</span > like office hours, classes and everything in between with the ultimate objective of helping startups avoid stupid mistakes, "None of this is to say that we have all the answers," explains Google Ventures Partner <a href="http: \/\/www.crunchbase.com\/person\/bill-maris">Bill Maris<\/a> "We\'re just saying \'This is how we\'ve done it. And if you\'re going to make a mistake, make a new one.\'"\n'
                        },
                        {
                            'title': 'YouTube Reportedly Forking Out $100M For TV-Esque Content',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/kYuJI2dPpTY\/',
                            'pubDate': 'Tue, 04 Oct 2011 02: 11: 15 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/frenz.jpg?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="frenz" title="frenz" style="float: left; margin: 0 10px 7px 0;" \/>The world\'s foremost video-sharing playground may be more or less enough for its users, who want nothing more than to put their freestyles and kitten videos online, but it\'s still on its way to becoming the money-making machine Google wants it to be. And while hamsters eating popcorn garner millions of views, they\'re not the easiest to sell targeted ads for, and that\'s where the money is.\n\nSo <a href="http: \/\/online.wsj.com\/article\/SB10001424052970204612504576609101775893100.html">according to the Wall Street Journal<\/a> and its mysterious "people familiar with the matter," YouTube is finalizing a big content push with studios and networks, laying out $100 million to get some exclusive and high-quality content. Also, something probably related to <a href="http: \/\/www.guardian.co.uk\/culture\/2009\/aug\/29\/screen-burn-x-factor">The X-Factor<\/a>.'
                        },
                        {
                            'title': 'Joypad Turns Your iPhone Into A Remote Control; Launches New SDK To Bring iOS Gaming To Apple TV',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/9LOta-1VnJw\/',
                            'pubDate': 'Tue, 04 Oct 2011 01: 07: 09 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/screen-shot-2011-10-03-at-4-38-18-pm.png?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="Screen shot 2011-10-03 at 4.38.18 PM" title="Screen shot 2011-10-03 at 4.38.18 PM" style="float: left; margin: 0 10px 7px 0;" \/>Touch devices have revolutionized mobile gaming, just like they\'ve revolutionized the mobile space as a whole. For mobile games, they\'ve introduced some amazing interfaces and controllers that make those that have had the fortune of playing Atari on console take a step back and pinch themselves. Of course, technology is now taking us toward a confluence of consoles and touch devices. Today, companies like Joypad Inc. are turning our iPhones into game controllers. <a href="http: \/\/itunes.apple.com\/us\/app\/joypad-game-controller\/id411422117?mt=8">Joypad\'s eponymous free app<\/a> syncs directly with iPad, Mac, and PC games over bluetooth or WiFi to let gamers use their smartphone as a controller to play their favorite games on various NES-style control pads.\n\nGame developers can also use Joypad\'s free SDKs, which take about 30 minutes to integrate, to add Joypad support to their games. With these SDKs, gamers can interact with their favorite games in ways that were before impossible. For example, as a gamer reaches a new level, or unlocks new features, developers can push a new button out to the player in realtime. This kind of functionality gives developers a new way to create custom layouts for controllers and forge a deeper integration with the playing experience.'
                        },
                        {
                            'title': 'HP\u2019s Autonomy Buyout Finalized And Official',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/iLtdLjOf9sY\/',
                            'pubDate': 'Tue, 04 Oct 2011 00: 11: 46 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/hp.jpg?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="hp" title="hp" style="float: left; margin: 0 10px 7px 0;" \/>HP just announced that it has <a href="http: \/\/www.hp.com\/hpinfo\/newsroom\/press\/2011\/111003xb.html">completed its takeover of British enterprise data handler Autonomy<\/a> under the terms specified last month. To wit: \u00a325.50 each for 213,421,299 shares, totaling just over 87% of the company. That\'s around $8.5 billion spent of the ~$10 billion offer that would have constituted a total buyout (Reuters <a href="http: \/\/www.reuters.com\/article\/2011\/10\/03\/us-hp-autonomy-idUSTRE79269E20111003">says<\/a> $12 billion).\n\nThe purchase price is seen by many as rather an overpayment, but the purchase was one of the keystones in Leo Apotheker\'s plan to restructure the company. Apotheker, of course, <a href="http: \/\/techcrunch.com\/2011\/09\/22\/its-official-at-hp-apotheker-is-out-meg-whitmen-named-president-and-ceo\/">left the company<\/a> not long ago just a few weeks ago to make way for the new CEO, Meg Whitman, taking with him some <a href="http: \/\/techcrunch.com\/2011\/09\/29\/nice-work-if-you-can-get-it-apotheker-leaves-hp-with-10m-severance\/">$10 million in stock and bonuses<\/a>.'
                        },
                        {
                            'title': 'Fitocracy\u2019s Web App Wants You To Get Up And Go Mobile',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/mjHpq0FX8zg\/',
                            'pubDate': 'Mon, 03 Oct 2011 23: 30: 57 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/ja4ey.png?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="Ja4EY" title="Ja4EY" style="float: left; margin: 0 10px 7px 0;" \/>The guys at <a href="http: \/\/www.fitocracy.com">Fitocracy<\/a>, the social fitness tracker, have been very busy <a href="http: \/\/techcrunch.com\/2011\/06\/28\/fitocracy-brings-games-and-social-to-your-workouts-invites-within\/">since we\'ve seen them last<\/a>. Though I\'m a fan of the service, sometimes using it can be hit or miss. Don\'t get me wrong -- it\'s absolutely great on a computer, but the site never really scaled well for mobile devices. Thankfully, that\'s no longer the case, as the company has announced that the <a href="http: \/\/blog.fitocracy.com\/post\/10993255833\/announcing-fitocracy-mobile">first version of their mobile web app has just gone online<\/a>.\n'
                        },
                        {
                            'title': 'MG Siegler Will Become Our Apple Columnist, And Join CrunchFund As A VC',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/aCmbxa8jBkw\/',
                            'pubDate': 'Mon, 03 Oct 2011 22: 20: 53 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/mg.jpeg?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="MG" title="MG" style="float: left; margin: 0 10px 7px 0;" \/>When I first reached out to MG Siegler to join TechCrunch two and a half years ago,  I knew he was an amazing talent who could help cement our place as the premier technology blog on the Internet.  Ever since Michael and I hired him, he has not disappointed.  As his <a href="http: \/\/techcrunch.com\/2009\/08\/06\/its-on-bing-jingle-guy-proves-he-sucks-less\/">TechCrunch power<\/a> has grown, others have noticed his talents as well.  Everyone from the New York Times to every one of our competitors has tried to recruit him away, but he\u2019s always stuck with TechCrunch.\n\nOver the past few months, a lot of venture capital firms have been trying to hire him as well.  This time, the lure was too great.  He decided to change careers and will become a VC, just like Michael Arrington.  In fact, Michael is the one who is hiring him as a general partner at the CrunchFund (beating out offers from several other top-tier VC firms MG was considering).  But MG will be the first to tell you that his decision to become a VC predates all the recent drama around <a href="http: \/\/techcrunch.com\/2011\/09\/12\/deciding-to-move-on\/">Michael leaving TechCrunch<\/a> (and I will let him tell you so himself in his <a href="http: \/\/parislemon.com\/post\/11005151954\/on-the-next-venture">own post<\/a>).  While MG will only be working full time as a writer at TechCrunch for another month, I am pleased to announce that he will continue to write for us after that on a regular basis as an outside columnist.'
                        },
                        {
                            'title': 'StumbleUpon Brings Its Tablet Experience To Android, Optimizes UI Across All Its Mobile Apps',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/wjnyXiyL3B0\/',
                            'pubDate': 'Mon, 03 Oct 2011 22: 10: 49 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/alice_williams_homepage.png?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="alice_williams_homepage" title="alice_williams_homepage" style="float: left; margin: 0 10px 7px 0;" \/>Content discovery platform <a href="http: \/\/www.stumbleupon.com">StumbleUpon<\/a>, which just launched a <a href="http: \/\/techcrunch.com\/2011\/07\/11\/stumbleupon-ipad\/">much improved iPad app in August<\/a>, now brings that same user experience to Android with the very first StumbleUpon app designed specifically for Android tablets (the app was previously only available for the Android phone). The app is ready for download in the Android Market <a href="https: \/\/market.android.com\/details?id=com.stumbleupon.android.app">here.<\/a>\n\nThe new StumbleUpon Android tablet has basically the same functionality as the new iPad app (even though at first run-through some of the swipe features seem faster on the iPad).'
                        },
                        {
                            'title': 'Upcoming Nikon D800 Said To Be 36-Megapixel, $4000 Monster',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/UXiXfzB-e8c\/',
                            'pubDate': 'Mon, 03 Oct 2011 22: 09: 59 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/d800.jpg?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="d800" title="d800" style="float: left; margin: 0 10px 7px 0;" \/>Some specs for Nikon\'s next semi-pro camera have surfaced on Japanese camera site <a href="http: \/\/translate.google.com\/translate?sl=ja&#38;tl=en&#38;js=n&#38;prev=_n&#38;hl=en&#38;zn=en&#38;ie=UTF-8&#38;layout=2&#38;eotf=1&#38;u=http%3A%2F%2Fdigicame-info.com%2F2011%2F09%2Fd800d7003600.html">Digital Camera Info<\/a>, and <a href="http: \/\/nikonrumors.com\/2011\/10\/03\/the-name-will-be-nikon-d800-the-sensor-will-be-36mp-99-probability.aspx\/">Nikon Rumors<\/a> seems to think they\'re creditable. It\'s an interesting move by Nikon but not one that will be appreciated by the bulk of DSLR buyers.\n\nThe new D800, they say, will pack 36 megapixels on a full frame FX sensor, and essentially forgo advances in low-light performance in order to produce a medium-cost studio camera instead of a lower-cost prosumer one. The $4000 price puts it out of most enthusiasts\' reach, and the high megapixel count makes it less practical for sports and field photography.'
                        },
                        {
                            'title': 'Apple To Battle iOS Piracy With App Rentals?',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/0Vy4JmiiERI\/',
                            'pubDate': 'Mon, 03 Oct 2011 21: 30: 22 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/floppy.jpeg?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="floppy" title="floppy" style="float: left; margin: 0 10px 7px 0;" \/>Be it wariness of compatibility or quality, a lack of disposable income, or some unnoticeable form of protest, people will always find a justification to pirate things. Of course, many of these justifications tend to be secondary to the realization that Not Paying For Stuff &#62; Paying For Stuff \u2014 but hey, I\'m in no position to pretend to be on any sort of moral pedestal here.\n\nWithin the iOS piracy scene, the most commonly cited justification seems to be something along the lines of "There\'s too much crap on the App Store! How will I know if an app is any good before I shell out a whole <em>dollar<\/em> for it? I\'m not pirating, I\'m testing!"\n\nWell, Installous fans, it looks like you might soon have to find a new bit of logic to absolve your conscience. Based on a couple of strings tucked within the latest iTunes Beta, it looks like Apple might be crackin\' away at a way for users to give iOS apps a spin before they pay full price, no-jailbreak-required.'
                        },
                        {
                            'title': 'HouseFix, The Carfax For Home Improvement, Wants To Find You The Best Local Contractors (Video)',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/Itj-1RT5h6o\/',
                            'pubDate': 'Mon, 03 Oct 2011 21: 21: 15 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/housefixlogo.png?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="housefixlogo" title="housefixlogo" style="float: left; margin: 0 10px 7px 0;" \/>Launching at TechCrunch Disrupt in San Francisco last month was a startup called <a href="http: \/\/www.housefix.com\/">HouseFix<\/a>. As you might be able to guess by its name, HouseFix is a startup and service that aims to help you find the right people to fix up your house. Started by homeowners and contractors, the startup is trying to make the process of finding skilled labor for home repairs better for both parties. To do this, HouseFix wants to give homeowners a CARFAX report for their house -- a complete history of facts about their house that includes a list of improvement projects or anything that future homeowners would find relevant. \n\nTo differentiate itself from other providers like Angie\'s List or ServiceMagic, HouseFix is bringing social and local contexts to homeowners in a simple and intuitive way, so that they can quickly identify local contractors that fit their personal criteria for what kind of labor they\'re looking for, pricing, etc. The startup wants you to be able to find contractors you can trust in a matter of minutes, not days or weeks. \n'
                        },
                        {
                            'title': 'With Shooting Caught On Officer\u2019s \u201cChest-Cam,\u201d Tech Precedent To Be Set',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/DruV3J0w8lU\/',
                            'pubDate': 'Mon, 03 Oct 2011 20: 11: 01 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/le2_onofficer.jpg?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="LE2_onofficer" title="LE2_onofficer" style="float: left; margin: 0 10px 7px 0;" \/>The rising number of cameras recording activity on the street and on the job makes for an interesting new set of problems. I examined a few in my <a href="http: \/\/techcrunch.com\/2011\/06\/17\/surveillant-society\/">Surveillant Society<\/a> post, and <a href="http: \/\/www.sfgate.com\/cgi-bin\/article.cgi?f=\/c\/a\/2011\/10\/03\/MNGR1LBKEQ.DTL">one has just emerged<\/a> that could set a serious precedent for the application of tech in criminal cases.\n\nOn September 25, an Oakland police officer pulled over a car and the suspect got out and fled. The officer chased him, and during a struggle the suspect was shot and killed. The charges, suspect\'s and officer\'s names, are undisclosed but it was stated that the suspect was armed with a gun.\n\nIt would be another sadly typical escalation with a lethal end, except that the officer in question had at some point flipped on his "chest-cam," a relatively recent development in policing where a Flip-type pocket cam (in this case a <a href="http: \/\/www.vievu.com\/">Vievu<\/a> model) is attached to the uniform and turned on under certain circumstances. The presence of this camera is leading to a few potentially major legal questions given the stakes of the case. While some are more legal than tech-related, it\'s worth taking a minute to see how technological advances are shaping criminal law.'
                        },
                        {
                            'title': 'Did Sprint Go All-In For The iPhone 5?',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/cBCpP6vru08\/',
                            'pubDate': 'Mon, 03 Oct 2011 19: 40: 34 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/06\/sprint-iphone.jpg?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="Image (1) sprint-iphone.jpg for post 47454" title="Image (1) sprint-iphone.jpg for post 47454" style="float: left; margin: 0 10px 7px 0;" \/>Sprint is taking a huge risk by carrying the iPhone, if reports from <a href="http: \/\/online.wsj.com\/article\/SB10001424052970203405504576603053795839250.html?mod=WSJ_hp_LEFTTopStories">the Wall Street Journal<\/a> are to be believed. Details about Sprint and Apple\'s quiet dealings have begun to emerge, and if true, the country\'s third largest wireless carrier is stuck in a precarious position.\n\nSprint CEO Dan Hesse has reportedly told the company\'s board of directors that in order to nab the iPhone, they would have to commit to purchasing at least 30.5 million iPhones over the next four years. It\'s a huge investment by any stretch -- WSJ pegs it at around $20 billion at current rates -- but it\'s made worse by the fact that Sprint will be locked into the purchasing agreement regardless of whether or not the iPhones actually sell. The board was said to have accepted Apple\'s terms.'
                        },
                        {
                            'title': 'PayPal To Open A Pop-Up Store In New York To Showcase New Payments Technologies',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/uau5p_HTctU\/',
                            'pubDate': 'Mon, 03 Oct 2011 19: 17: 52 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/paypal.png?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="paypal" title="paypal" style="float: left; margin: 0 10px 7px 0;" \/><strong>Exclusive<\/strong>-PayPal is going to be setting up a pop-up store in downtown Manhattan, New York to showcase some of the new tools and technologies the payments giant will debut in the next few months.\n\nThe space will be located at 174 Hudson, which is located in the Tribeca neighborhood of Manhattan. Over the next 3 and a half months, PayPal will be inviting merchants to come visit the space where they will have the opportunity to get real-time demos of the technologies in realistic point of sale scenarios. The store, which will launch on November 1, will also feature a QR code for people passing by to scan and see more details on the PayPal\'s new technologies.\n \n'
                        },
                        {
                            'title': 'Sean Parker Is Now On Twitter: \u201cSorry Zuck\u201d',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/r4CUTSRi1cc\/',
                            'pubDate': 'Mon, 03 Oct 2011 19: 00: 49 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/parker.png?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="parker" title="parker" style="float: left; margin: 0 10px 7px 0;" \/><a href="http: \/\/www.crunchbase.com\/person\/sean-parker">Sean Parker <\/a>is getting a lot louder.\n\nYou\'ve been hearing <em>about<\/em> him for a decade now \u2014 cofounder of Napster and Plaxo, founding President of Facebook, and most recently as an investor (and advocate) of Spotify. And he hasn\'t pulled any punches during his <a href="http: \/\/techcrunch.com\/2011\/06\/28\/sean-parker-on-why-myspace-lost-to-facebook\/">conference<\/a> appearances or interviews, either.'
                        },
                        {
                            'title': 'Adobe Pushes Into Tablet Space With 6 New Apps And \u201cCreative Cloud\u201d',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/sZz2R-s_DmE\/',
                            'pubDate': 'Mon, 03 Oct 2011 19: 00: 13 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/adobe-photoshop-touch.jpg?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="Adobe Photoshop Touch" title="Adobe Photoshop Touch" style="float: left; margin: 0 10px 7px 0;" \/>Adobe\'s had a busy day. Along with the <a href="http: \/\/techcrunch.com\/2011\/10\/03\/adobe-acquires-developer-of-html5-mobile-app-framework-phonegap-nitobi\/">acquisition of Nitobi Software<\/a> and TypeKit, the company has also made a clear push into the tablet space, looking to bolster content creation. At the Adobe MAX 2011 conference in Los Angeles, Adobe officially announced the Creative Cloud &#8212; its very own cloud storage offering &#8212; along with with six new Adobe Touch apps for Android tablets and the iPad. \n\nCreative Cloud lets users sync, share and view files from both the Adobe Creative Suite (desktop) and the Adobe Touch apps. It offers 20GB of free storage, though pricing and availability won\'t be announced until November. However, the Creative Cloud (once it\'s in action) will certainly boost the value of the six new Touch apps, most notable of which is Adobe Photoshop Touch. '
                        },
                        {
                            'title': 'Keen On\u2026 Why Women Excel in Social Media (TCTV)',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/uNL4AzZhJl8\/',
                            'pubDate': 'Mon, 03 Oct 2011 18: 15: 55 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/screen-shot-2011-10-03-at-10-46-59-am.png?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="Screen shot 2011-10-03 at 10.46.59 AM" title="Screen shot 2011-10-03 at 10.46.59 AM" style="float: left; margin: 0 10px 7px 0;" \/>It had to happen. Social media has, so to speak, been genderized. New on this week\u2019s <a href="http: \/\/online.wsj.com\/article\/SB10001424053111904265504576566661009105054.html?mod=WSJ_Books_LS_Books_13">best-selling non-fiction list<\/a> is  <a href="http: \/\/www.crunchbase.com\/person\/vicki-milazzo-2">Vickie Milazzo\u2019s<\/a> <a href="http: \/\/www.amazon.com\/Wicked-Success-Inside-Every-Woman\/dp\/1118100522">Wicked Success Is Inside Every Woman<\/a>, a self-help book for females which, in part, argues that women use social media more effectively than men.\n\nAs Vickie Milazzo told me when we Skyped late last week, women \u201cexcel at relationship building\u201d because they \u201cpretty much manage everything\u201d in the household. In contrast with men \u2013 who tend, she says, to see collaboration as a weakness \u2013 women are natural relationship builders who thus naturally excel at social media. And that\u2019s because, Milazzo explained to me, women use both sides of their brains while men only use one side of theirs.\n'
                        },
                        {
                            'title': 'Zynga Goes 3D With Crime World Facebook Game Mafia Wars 2; Launches In A Record 16 Languages',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/STqaS_bdww0\/',
                            'pubDate': 'Mon, 03 Oct 2011 18: 00: 22 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/mafiawars2.png?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="MafiaWars2" title="MafiaWars2" style="float: left; margin: 0 10px 7px 0;" \/>As we reported a few weeks ago, Zynga is <a href="http: \/\/techcrunch.com\/2011\/09\/20\/zynga-debuts-new-version-of-3-d-facebook-game-mafia-wars\/">debuting<\/a> a new version of its hit Facebook game Mafia Wars. Today, the social gaming giant is revealing all the details of the new, mobster-themed, crime-focused social game, <a href="http: \/\/mafiawars2.com\/">Mafia Wars 2.<\/a> The game itself is not live now, but will be available on Facebook within the next few weeks.\n\nOriginally launched 2008, Mafia Wars allows players to build their virtual criminal empires by collaborating with their friends to complete crime jobs, fight and rob other Mafia crews, run underground businesses and purchase virtual goods like weapons and getaway cars. Set in New York City at launch, the game expanded to new in-game locales including Cuba, Moscow, Bangkok, Las Vegas, Italy, Chicago, and Brazil in March 2011.\n\n'
                        },
                        {
                            'title': 'Rhapsody To Acquire Streaming Music Competitor Napster',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/MemXkioX0u0\/',
                            'pubDate': 'Mon, 03 Oct 2011 17: 43: 05 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/rhapsody_logo.jpg?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="rhapsody_logo" title="rhapsody_logo" style="float: left; margin: 0 10px 7px 0;" \/>On-demand music service Rhapsody will acquire competitor Napster in an effort to expand its user base, the companies have announced. Under the terms of the agreement, Rhapsody will acquire all Napster subscribers and certain other assets. Meanwhile, Best Buy, Napster\'s current owner, will receive a minority stake in Rhapsody.'
                        },
                        {
                            'title': '\u2018Invisible Key\u2019 Lets You Unlocks Doors With Hand Gestures',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/HRjqv08rHos\/',
                            'pubDate': 'Mon, 03 Oct 2011 17: 37: 56 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/hand-sign_1_md.gif?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="hand-sign_1_md" title="hand-sign_1_md" style="float: left; margin: 0 10px 7px 0;" \/>Forget about fumbling with your keys late at night -- a faculty member from the Technology and Science Institute  of Northern Taiwan has developed a way to unlock your door with <a href="http: \/\/www.geek.com\/articles\/geek-cetera\/invisible-key-system-unlocks-doors-with-a-hand-gesture-2011103\/">a simple hand gesture.<\/a>\n\nDeveloped by Tsai Yao-Pin and his team of researchers, the "invisible key" is actually a bit of a misnomer, as it\'s what\'s built into the lock that does all the heavy lifting. The heart of the invisible key is a special chip-and-accerometer combo that Yao-Pin and his team of researchers have developed. '
                        },
                        {
                            'title': 'Fuse Corps Will Pair Entrepreneurs With Government, Hopes To Achieve Large-Scale Social Change',
                            'link': 'http: \/\/feedproxy.google.com\/~r\/Techcrunch\/~3\/9KPbtsxzEyw\/',
                            'pubDate': 'Mon, 03 Oct 2011 17: 00: 30 +0000',
                            'description': '<img width="100" height="70" src="http: \/\/tctechcrunch2011.files.wordpress.com\/2011\/10\/fusecorpslogo.png?w=100&amp;h=70&amp;crop=1" class="attachment-tc-carousel-river-thumb wp-post-image" alt="fusecorpslogo" title="fusecorpslogo" style="float: left; margin: 0 10px 7px 0;" \/>New social venture <a href="http: \/\/fusecorps.org\/">Fuse Corps<\/a> is launching a platform to connect entrepreneurial professionals with governors, mayors and community leaders across the U.S. in an effort drive meaningful social change. The group will identify projects in various communities that address a national issue like education, health care or economic development, then recruit and pair a selected entrepreneur with those working in the public sector to collaborate on the project.'
                        }
                    ]
                }
            }
        };
    }

    suite.add(new YUITest.TestCase({

        name: 'rss model tests',

        setUp: function() {
            rss = Y.mojito.models["read-model-rss"];
        },

        tearDown: function() {
            rss = null;
        },

        'empty response': function() {
            A.areSame(0, rss.test.processResponse({}).length);
        },

        'process 20 in, 10 out': function() {
            A.areSame(20, mockresp().query.results.item.length);
            A.areSame(10, rss.test.processResponse(mockresp(), 10).length);
        },

        'skipped 2 stories': function() {
            var resp = mockresp();
            resp.query.results.item = resp.query.results.item.slice(10);
            A.areSame(10, resp.query.results.item.length);
            resp.query.results.item.pop();
            delete resp.query.results.item[0].title;
            A.areSame(8, rss.test.processResponse(resp).length);
        },

        'processError exists': function() {
            A.areSame('no foo in your bar', rss.test.processError({query: {diagnostics: {url: {'http-status-message': 'no foo in your bar'}}}}));
        },

        'processError not exists is falsey': function() {
            A.isFalse(!!rss.test.processError({query: {}}));
            A.isFalse(!!rss.test.processError({}));
            A.isFalse(!!rss.test.processError(''));
        },

        'processResponse strips tags': function() {
            var list = rss.test.processResponse(mockresp());
            /*jslint unparam:true*/
            Y.each(list, function(story, i) {
                A.areNotEqual(mockresp().query.results.item[i].description, list[i].description);
            });
        },

        'strip tags': function() {
            A.areSame(
                'no  tags\ntags  found  or',
                rss.test.stripTags('  no <b>tags\ntags</b> found <here style="asasda">or</here > ')
            );
        }

    }));

    YUITest.TestRunner.add(suite);

}, '0.0.1', {requires: [
    'mojito-test',
    'read-model-rss'
]});
