import {int} from 'angular2/src/facade/lang';
import {Math} from 'angular2/src/facade/math';

import {PromiseWrapper} from 'angular2/src/facade/async';
import {ListWrapper, MapWrapper} from 'angular2/src/facade/collection';

export var ITEMS = 1000;
export var ITEM_HEIGHT = 40;
export var VISIBLE_ITEMS = 17;

export var HEIGHT = ITEMS * ITEM_HEIGHT;
export var VIEW_PORT_HEIGHT = ITEM_HEIGHT * VISIBLE_ITEMS;

export var COMPANY_NAME_WIDTH = 100;
export var OPPORTUNITY_NAME_WIDTH = 100;
export var OFFERING_NAME_WIDTH = 100;
export var ACCOUNT_CELL_WIDTH = 50;
export var BASE_POINTS_WIDTH = 50;
export var KICKER_POINTS_WIDTH = 50;
export var STAGE_BUTTONS_WIDTH = 220;
export var BUNDLES_WIDTH = 120;
export var DUE_DATE_WIDTH = 100;
export var END_DATE_WIDTH = 100;
export var AAT_STATUS_WIDTH = 100;
export var ROW_WIDTH = COMPANY_NAME_WIDTH +
  OPPORTUNITY_NAME_WIDTH +
  OFFERING_NAME_WIDTH +
  ACCOUNT_CELL_WIDTH +
  BASE_POINTS_WIDTH +
  KICKER_POINTS_WIDTH +
  STAGE_BUTTONS_WIDTH +
  BUNDLES_WIDTH +
  DUE_DATE_WIDTH +
  END_DATE_WIDTH +
  AAT_STATUS_WIDTH;

export var STATUS_LIST = [
    'Planned', 'Pitched', 'Won', 'Lost'
];

export var AAT_STATUS_LIST = [
    'Active', 'Passive', 'Abandoned'
];

// Imitate Streamy entities.

// Just a non-trivial object. Nothing fancy or correct.
export class CustomDate {
  year: int;
  month: int;
  day: int;

  constructor(y:int, m:int, d:int) {
    this.year = y;
    this.month = m;
    this.day = d;
  }

  addDays(days:int):CustomDate {
    var newDay = this.day + days;
    var newMonth = this.month + Math.floor(newDay / 30);
    newDay = newDay % 30;
    var newYear = this.year + Math.floor(newMonth / 12);
    return new CustomDate(newYear, newMonth, newDay);
  }

  static now():CustomDate {
    return new CustomDate(2014, 1, 28);
  }
}

export class RawEntity {

  _data:Map;

  constructor() {
    this._data = MapWrapper.create();
  }

  get(key:string) {
    if (key.indexOf('.') == -1) {
      return this._data[key];
    }
    var pieces = key.split('.');
    var last = ListWrapper.last(pieces);
    pieces.length = pieces.length - 1;
    var target = _resolve(pieces, this);
    if (target == null) {
      return null;
    }
    return target[last];
  }

  set(key:string, value) {
    if (key.indexOf('.') == -1) {
      this._data[key] = value;
      return;
    }
    var pieces = key.split('.');
    var last = ListWrapper.last(pieces);
    pieces.length = pieces.length - 1;
    var target = _resolve(pieces, this);
    target[last] = value;
  }

  remove(key:string) {
    if (!key.contains('.')) {
      return MapWrapper.delete(this._data, key);
    }
    var pieces = key.split('.');
    var last = ListWrapper.last(pieces);
    pieces.length = pieces.length - 1;
    var target = _resolve(pieces, this);
    return target.remove(last);
  }

  _resolve(pieces, start) {
    var cur = start;
    for (var i = 0; i < pieces.length; i++) {
      cur = cur[pieces[i]];
      if (cur == null) {
        return null;
      }
    }
    return cur;
  }
}

export class Company extends RawEntity {
  get name():string { return this.get('name'); }
  set name(val:string) {
    this.set('name', val);
  }
}

export class Offering extends RawEntity {
  get name():string { return this.get('name'); }
  set name(val:string) {
    this.set('name', val);
  }

  get company():Company { return this.get('company'); }
  set company(val:Company) {
    this.set('company', val);
  }

  get opportunity():Opportunity { return this.get('opportunity'); }
  set opportunity(val:Opportunity) {
    this.set('opportunity', val);
  }

  get account():Account { return this.get('account'); }
  set account(val:Account) {
    this.set('account', val);
  }

  get basePoints():int { return this.get('basePoints'); }
  set basePoints(val:int) {
    this.set('basePoints', val);
  }

  get kickerPoints():int { return this.get('kickerPoints'); }
  set kickerPoints(val:int) {
    this.set('kickerPoints', val);
  }

  get status():string { return this.get('status'); }
  set status(val:string) {
    this.set('status', val);
  }

  get bundles():string { return this.get('bundles'); }
  set bundles(val:string) {
    this.set('bundles', val);
  }

  get dueDate():CustomDate { return this.get('dueDate'); }
  set dueDate(val:CustomDate) {
    this.set('dueDate', val);
  }

  get endDate():CustomDate { return this.get('endDate'); }
  set endDate(val:CustomDate) {
    this.set('endDate', val);
  }

  get aatStatus():string { return this.get('aatStatus'); }
  set aatStatus(val:string) {
    this.set('aatStatus', val);
  }
}

export class Opportunity extends RawEntity {
  get name():string { return this.get('name'); }
  set name(val:string) {
    this.set('name', val);
  }
}

export class Account extends RawEntity {
  get accountId():int { return this.get('accountId'); }
  set accountId(val:int) {
    this.set('accountId', val);
  }
}
