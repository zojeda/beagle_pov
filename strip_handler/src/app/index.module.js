/* global malarkey:false, toastr:false, moment:false */
import config from './index.config';

import runBlock from './index.run';
import MainController from './main/main.controller';
import NavbarDirective from '../app/components/navbar/navbar.directive';

angular.module('beaglePov', ['ngAnimate', 'ngCookies', 'ngTouch', 'ngSanitize', 'ui.bootstrap'])
  .constant('malarkey', malarkey)
  .constant('toastr', toastr)
  .constant('moment', moment)
  .config(config)

  .run(runBlock)
  .controller('MainController', MainController)
  .directive('acmeNavbar', () => new NavbarDirective());
