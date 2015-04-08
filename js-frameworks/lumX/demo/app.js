/* global angular */
/* global escape */
'use strict'; // jshint ignore:line

angular
    .module('lumx-demo', [
        'ngRoute',
        'lumx',
        'hljs',
        'Services'
    ])
    .config(function($locationProvider, $routeProvider)
    {
        $locationProvider.html5Mode({
            enabled: true,
            requireBase: false
        });

        $routeProvider.
            when('/', {
                templateUrl: '/includes/homepage/homepage.html'
            }).
            when('/getting-started/installation', {
                templateUrl: '/includes/start/installation.html'
            }).
            when('/getting-started/customization', {
                templateUrl: '/includes/start/customization.html'
            }).
            when('/getting-started/contribute', {
                templateUrl: '/includes/start/contribute.html'
            }).
            when('/css/mixins', {
                templateUrl: '/includes/css/mixins.html'
            }).
            when('/css/typography', {
                templateUrl: '/includes/css/typography.html'
            }).
            when('/css/colors', {
                templateUrl: '/includes/css/colors.html'
            }).
            when('/css/flexbox', {
                templateUrl: '/includes/modules/flexbox/flexbox.html'
            }).
            when('/css/buttons', {
                templateUrl: '/includes/modules/button/button.html'
            }).
            when('/css/fab', {
                templateUrl: '/includes/modules/fab/fab.html'
            }).
            when('/css/icons', {
                templateUrl: '/includes/modules/icon/icon.html'
            }).
            when('/css/lists', {
                templateUrl: '/includes/modules/list/list.html'
            }).
            when('/css/data-table', {
                templateUrl: '/includes/modules/data-table/data-table.html'
            }).
            when('/css/cards', {
                templateUrl: '/includes/modules/card/card.html'
            }).
            when('/css/checkboxes', {
                templateUrl: '/includes/modules/checkbox/checkbox.html'
            }).
            when('/css/radio-buttons', {
                templateUrl: '/includes/modules/radio-button/radio-button.html'
            }).
            when('/css/switches', {
                templateUrl: '/includes/modules/switch/switch.html'
            }).
            when('/css/toolbars', {
                templateUrl: '/includes/modules/toolbar/toolbar.html'
            }).
            when('/directives/dropdowns', {
                templateUrl: '/includes/modules/dropdown/dropdown.html'
            }).
            when('/directives/tabs', {
                templateUrl: '/includes/modules/tabs/tabs.html'
            }).
            when('/directives/text-fields', {
                templateUrl: '/includes/modules/text-field/text-field.html'
            }).
            when('/directives/search-filter', {
                templateUrl: '/includes/modules/search-filter/search-filter.html'
            }).
            when('/directives/selects', {
                templateUrl: '/includes/modules/select/select.html'
            }).
            when('/directives/file-inputs', {
                templateUrl: '/includes/modules/file-input/file-input.html'
            }).
            when('/directives/tooltips', {
                templateUrl: '/includes/modules/tooltip/tooltip.html'
            }).
            when('/directives/dialogs', {
                templateUrl: '/includes/modules/dialog/dialog.html'
            }).
            when('/directives/date-picker', {
                templateUrl: '/includes/modules/date-picker/date-picker.html'
            }).
            when('/directives/thumbnails', {
                templateUrl: '/includes/modules/thumbnail/thumbnail.html'
            }).
            when('/directives/scrollbar', {
                templateUrl: '/includes/modules/scrollbar/scrollbar.html'
            })
            .when('/services/notifications', {
                templateUrl: '/includes/modules/notification/notification.html'
            })
            .when('/services/progress', {
                templateUrl: '/includes/modules/progress/progress.html'
            });
    })
    .controller('AppController', function($http, $scope, $location, LxNotificationService, LxDialogService, LxProgressService, Sidebar)
    {
        $scope.Sidebar = Sidebar;

        $scope.checkHome = function()
        {
            return $location.path() === '/';
        };

        $scope.repo = {
            lastCommit: {
                date: undefined,
                sha: undefined,
                url: undefined
            },
            lastRelease: {
                name: undefined,
                tag: undefined
            }
        };

        $http.get('https://api.github.com/repos/lumapps/lumx/git/refs/heads/master').success(function(master)
        {
            $http.get(master.object.url).success(function(lastCommit)
            {
                $scope.repo.lastCommit.date = new Date(lastCommit.author.date);
                $scope.repo.lastCommit.sha = lastCommit.sha;
                $scope.repo.lastCommit.url = lastCommit.html_url;
            });
        });

        $http.get('https://api.github.com/repos/lumapps/lumx/releases').success(function(lastReleases)
        {
            var lastRelease = lastReleases[0] ? lastReleases[0] : lastReleases;
            $scope.repo.lastRelease.name = lastRelease.name;
            $scope.repo.lastRelease.tag = lastRelease.tag_name;
            $scope.repo.lastRelease.url = lastRelease.zipball_url;
        });

        $scope.people = [
            { name: 'Adam',      email: 'adam@email.com',      age: 10 },
            { name: 'Amalie',    email: 'amalie@email.com',    age: 12 },
            { name: 'Wladimir',  email: 'wladimir@email.com',  age: 30 },
            { name: 'Samantha',  email: 'samantha@email.com',  age: 31 },
            { name: 'Estefanía', email: 'estefanía@email.com', age: 16 },
            { name: 'Natasha',   email: 'natasha@email.com',   age: 54 },
            { name: 'Nicole',    email: 'nicole@email.com',    age: 43 },
            { name: 'Adrian',    email: 'adrian@email.com',    age: 21 }
        ];

        $scope.selectSections = {
            'Sub header 1': [
                { uid: '1', name: 'Adam' },
                { uid: '2', name: 'Amalie' },
                { uid: '3', name: 'Wladimir' },
                { uid: '4', name: 'Samantha' }
            ],
            '<i class="mdi mdi-android"></i> Sub header 2': [
                { uid: '5', name: 'Estefanía' },
                { uid: '6', name: 'Natasha' },
                { uid: '7', name: 'Nicole' }
            ]
        };

        $scope.ajax = {
            selected: 'Inception',
            list: [],
            update: function(newFilter)
            {
                if (newFilter)
                {
                    $scope.ajax.loading = true;
                    $http.get('http://www.omdbapi.com/?s=' + escape(newFilter)).
                        success(function(data)
                        {
                            $scope.ajax.list = data.Search;
                            $scope.ajax.loading = false;
                        }).
                        error(function()
                        {
                            $scope.ajax.loading = false;
                        });
                }
                else
                {
                    $scope.ajax.list = false;
                }
            },
            toModel: function(data, callback)
            {
                if (data)
                {
                    callback(data.Title);
                }
                else
                {
                    callback();
                }
            },
            toSelection: function(data, callback)
            {
                if (data)
                {
                    $http.get('http://www.omdbapi.com/?s=' + escape(data)).
                        success(function(response)
                        {
                            callback(response.Search[0]);
                        }).
                        error(function()
                        {
                            callback();
                        });
                }
                else
                {
                    callback();
                }
            },
            loading: false
        };

        $scope.cbSelect = {
            exec: function(newVal, oldVal)
            {
                LxNotificationService.notify('Change detected!');
                console.log('oldVal: ', oldVal);
                console.log('newVal: ', newVal);
            }
        };

        $scope.selects = {
            selectedPerson: undefined,
            selectedPersons: [$scope.people[2], $scope.people[4]],
            selectedPersons2: []
        };

        $scope.tree = [
        {
            id: 1,
            children: [
            {
                id: 11,
                children: [{id: 111}]
            },
            {
                id: 12,
                children: [{id: 121}, {id: 122}]
            },
            {
                id: 13
            }]
        },
        {
            id: 2
        },
        {
            id: 3,
            children: [
            {
                id: 32,
                children: [{id: 321}, {id: 322}, {id: 323}]
            }]
        }];

        $scope.selectedTags = [$scope.people[1], $scope.people[2]];

        $scope.scrollbarHeight = 150;
        $scope.scrollbarContent = 'Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi commodo varius nisl, quis tempor nunc scelerisque eget. Nam ultricies nulla et felis sollicitudin, eget facilisis neque fringilla. Aliquam malesuada, massa sit amet vehicula ultrices, erat tortor pretium enim, at mattis enim tellus eget felis. Duis euismod mollis ligula nec commodo. Aenean laoreet molestie ex id porta. Etiam vitae libero ac augue pellentesque tempor at id felis. Donec finibus purus non tortor sollicitudin consequat. Curabitur sit amet tincidunt odio. Pellentesque in accumsan nibh. Nulla quis rutrum lectus. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi commodo varius nisl, quis tempor nunc scelerisque eget. Nam ultricies nulla et felis sollicitudin, eget facilisis neque fringilla. Aliquam malesuada, massa sit amet vehicula ultrices, erat tortor pretium enim, at mattis enim tellus eget felis. Duis euismod mollis ligula nec commodo. Aenean laoreet molestie ex id porta. Etiam vitae libero ac augue pellentesque tempor at id felis. Donec finibus purus non tortor sollicitudin consequat. Curabitur sit amet tincidunt odio. Pellentesque in accumsan nibh. Nulla quis rutrum lectus. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi commodo varius nisl, quis tempor nunc scelerisque eget. Nam ultricies nulla et felis sollicitudin, eget facilisis neque fringilla. Aliquam malesuada, massa sit amet vehicula ultrices, erat tortor pretium enim, at mattis enim tellus eget felis. Duis euismod mollis ligula nec commodo. Aenean laoreet molestie ex id porta. Etiam vitae libero ac augue pellentesque tempor at id felis. Donec finibus purus non tortor sollicitudin consequat. Curabitur sit amet tincidunt odio. Pellentesque in accumsan nibh. Nulla quis rutrum lectus. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi commodo varius nisl, quis tempor nunc scelerisque eget. Nam ultricies nulla et felis sollicitudin, eget facilisis neque fringilla. Aliquam malesuada, massa sit amet vehicula ultrices, erat tortor pretium enim, at mattis enim tellus eget felis. Duis euismod mollis ligula nec commodo. Aenean laoreet molestie ex id porta. Etiam vitae libero ac augue pellentesque tempor at id felis. Donec finibus purus non tortor sollicitudin consequat. Curabitur sit amet tincidunt odio. Pellentesque in accumsan nibh. Nulla quis rutrum lectus.';

        $scope.dropdownAlert = function()
        {
            LxNotificationService.info('Clicked!');
        };

        $scope.changeScrollbarHeight = function(height)
        {
            $scope.scrollbarHeight = height;
        };

        $scope.changeScrollbarContent = function(content)
        {
            $scope.scrollbarContent = content;
        };

        $scope.opendDialog = function(dialogId)
        {
            LxDialogService.open(dialogId);
        };

        $scope.closingDialog = function()
        {
            LxNotificationService.info('Dialog closed!');
        };

        $scope.showCircularProgress = function()
        {
            LxProgressService.circular.show('#5fa2db', '#progress');
        };

        $scope.showSmallCircularProgress = false;

        $scope.toggleSmallCircularProgress = function()
        {
            $scope.showSmallCircularProgress = !$scope.showSmallCircularProgress;
        };

        $scope.hideCircularProgress = function()
        {
            LxProgressService.circular.hide();
        };

        $scope.showLinearProgress = function()
        {
            LxProgressService.linear.show('#5fa2db', '#progress');
        };

        $scope.hideLinearProgress = function()
        {
            LxProgressService.linear.hide();
        };

        $scope.searchFilter = {
            first: '',
            second: '',
            third: '',
            fourth: ''
        };

        $scope.textFields = {
            disabled: 'LumApps',
            firstName: 'Leeloo',
            lastName: '',
            firstEmail: 'bad-email',
            secondEmail: 'hello@lumapps.com',
            thirdEmail: 'hello@lumapps.com',
            description: '',
            disabledDark: 'LumApps',
            firstNameDark: 'Leeloo',
            lastNameDark: '',
            firstEmailDark: 'bad-email',
            secondEmailDark: 'hello@lumapps.com',
            thirdEmailDark: 'hello@lumapps.com',
            descriptionDark: ''
        };

        $scope.emailValidation = function(email)
        {
            return /^[0-9a-zA-Z]+([0-9a-zA-Z]*[-._+])*[0-9a-zA-Z]+@[0-9a-zA-Z]+([-.][0-9a-zA-Z]+)*([0-9a-zA-Z]*[.])[a-zA-Z]{2,6}$/.test(email);
        };

        $scope.$watch('searchFilter.first', function(newVal, oldVal)
        {
            console.log("Filter changed: '" + newVal + "' from '" + oldVal + "'");
        });

        $scope.datepicker = {
            date: new Date()
        };

        var tabIndex = 4;
        $scope.tabs = [
            { heading: 'Tab 1', content: 'Tab 1 content' },
            { heading: 'Tab 2', content: 'Tab 2 content' },
            { heading: 'Tab 3', content: 'Tab 3 content' }
        ];

        $scope.addTab = function()
        {
            $scope.tabs.push({ heading: 'Tab ' + tabIndex, content: 'Tab ' + tabIndex + ' content' });
            ++tabIndex;
        };

        $scope.removeFirstTab = function()
        {
            $scope.removeTab(0);
        };

        $scope.removeTab = function(idx)
        {
            if ($scope.tabs.length > idx)
            {
                $scope.tabs.splice(idx, 1);
            }
        };

        $scope.addPerson = function()
        {
            $scope.people.push({ name: 'Lorem', email: 'lorem@email.com', age: 99 })
        };

        $scope.thumbnail = '/images/placeholder/1-horizontal.jpg';
        $scope.thumbnailWidth = 60;
        $scope.thumbnailHeight = 60;

        $scope.updateThumbnail = function()
        {
            $scope.thumbnail = '/images/placeholder/2-vertical.jpg';
            $scope.thumbnailWidth = 150;
            $scope.thumbnailHeight = 80;
        };

        $scope.notify = function(type)
        {
            if (type === 'simple')
            {
                LxNotificationService.notify('Lorem Ipsum');
            }
            else if (type === 'sticky')
            {
                LxNotificationService.notify('Lorem Ipsum', undefined, true);
            }
            else if (type === 'icon')
            {
                LxNotificationService.notify('Lorem Ipsum', 'android');
            }
            else if (type === 'color')
            {
                LxNotificationService.notify('Lorem Ipsum', 'android', false, 'green');
            }
            else if (type === 'info')
            {
                LxNotificationService.info('Lorem Ipsum');
            }
            else if (type === 'success')
            {
                LxNotificationService.success('Lorem Ipsum');
            }
            else if (type === 'warning')
            {
                LxNotificationService.warning('Lorem Ipsum');
            }
            else if (type === 'error')
            {
                LxNotificationService.error('Lorem Ipsum');
            }
        };

        $scope.alert = function()
        {
            LxNotificationService.alert('Lorem Ipsum', 'Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praesent sit amet urna quis nisi sodales semper pharetra eu augue.', 'Ok', function(answer)
            {
                console.log(answer);
            });
        };

        $scope.confirm = function()
        {
            LxNotificationService.confirm('Lorem Ipsum', 'Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praesent sit amet urna quis nisi sodales semper pharetra eu augue.', { cancel:'Disagree', ok:'Agree' }, function(answer)
            {
                console.log(answer);
            });
        };
    });

angular
    .module('Services', [])
    .service('Sidebar', function()
    {
        var sidebarIsShown = false;

        function toggleSidebar()
        {
            sidebarIsShown = !sidebarIsShown;
        }

        return {
            isSidebarShown: function()
            {
                return sidebarIsShown;
            },
            toggleSidebar: toggleSidebar
        };
    });
