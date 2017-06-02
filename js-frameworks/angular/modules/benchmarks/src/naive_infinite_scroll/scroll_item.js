import {int} from 'angular2/src/facade/lang';
import {reflector} from 'angular2/src/reflection/reflection';
import {Component, Viewport, Template, ViewContainer, Compiler}
    from 'angular2/angular2';
import {PromiseWrapper} from 'angular2/src/facade/async';
import {ListWrapper, MapWrapper} from 'angular2/src/facade/collection';
import {DOM} from 'angular2/src/dom/dom_adapter';
import {Math} from 'angular2/src/facade/math';
import {CompanyNameComponent, OpportunityNameComponent,
    OfferingNameComponent, StageButtonsComponent, AccountCellComponent,
    FormattedCellComponent} from './cells';

import {Offering, ITEM_HEIGHT, COMPANY_NAME_WIDTH, OPPORTUNITY_NAME_WIDTH,
    OFFERING_NAME_WIDTH, ACCOUNT_CELL_WIDTH, BASE_POINTS_WIDTH,
    KICKER_POINTS_WIDTH, STAGE_BUTTONS_WIDTH, BUNDLES_WIDTH, DUE_DATE_WIDTH,
    END_DATE_WIDTH, AAT_STATUS_WIDTH} from './common';
import {generateOfferings} from './random_data';

export class ScrollItemComponent {

    offering:Offering;
    itemStyle;

    constructor() {
      this.itemStyle = MapWrapper.createFromPairs([
        ['height', `${ITEM_HEIGHT}px`],
        ['line-height', `${ITEM_HEIGHT}px`],
        ['font-size', '18px'],
        ['display', 'flex'],
        ['justify-content', 'space-between']
      ]);
    }

    get companyNameWidth() { return `${COMPANY_NAME_WIDTH}px`; }
    get opportunityNameWidth() { return `${OPPORTUNITY_NAME_WIDTH}px`; }
    get offeringNameWidth() { return `${OFFERING_NAME_WIDTH}px`; }
    get accountCellWidth() { return `${ACCOUNT_CELL_WIDTH}px`; }
    get basePointsWidth() { return `${BASE_POINTS_WIDTH}px`; }
    get kickerPointsWidth() { return `${KICKER_POINTS_WIDTH}px`; }
    get stageButtonsWidth() { return `${STAGE_BUTTONS_WIDTH}px`; }
    get bundlesWidth() { return `${BUNDLES_WIDTH}px`; }
    get dueDateWidth() { return `${DUE_DATE_WIDTH}px`; }
    get endDateWidth() { return `${END_DATE_WIDTH}px`; }
    get aatStatusWidth() { return `${AAT_STATUS_WIDTH}px`; }
}

export function setupReflectorForScrollItem() {
  reflector.registerType(ScrollItemComponent, {
    'factory': () => new ScrollItemComponent(),
    'parameters': [],
    'annotations': [
      new Component({
        selector: 'scroll-item',
        bind: {
          'offering': 'offering'
        }
      }),
      new Template({
        directives: [
          CompanyNameComponent,
          OpportunityNameComponent,
          OfferingNameComponent,
          StageButtonsComponent,
          AccountCellComponent,
          FormattedCellComponent
        ],
        inline: `
          <div class="row" [style]="itemStyle">
              <company-name [company]="offering.company"
                            [cell-width]="companyNameWidth">
              </company-name>
              <opportunity-name [opportunity]="offering.opportunity"
                                [cell-width]="opportunityNameWidth">
              </opportunity-name>
              <offering-name [offering]="offering"
                             [cell-width]="offeringNameWidth">
              </offering-name>
              <account-cell [account]="offering.account"
                            [cell-width]="accountCellWidth">
              </account-cell>
              <formatted-cell [value]="offering.basePoints"
                              [cell-width]="basePointsWidth">
              </formatted-cell>
              <formatted-cell [value]="offering.kickerPoints"
                              [cell-width]="kickerPointsWidth">
              </formatted-cell>
              <stage-buttons [offering]="offering"
                             [cell-width]="stageButtonsWidth">
              </stage-buttons>
              <formatted-cell [value]="offering.bundles"
                              [cell-width]="bundlesWidth">
              </formatted-cell>
              <formatted-cell [value]="offering.dueDate"
                              [cell-width]="dueDateWidth">
              </formatted-cell>
              <formatted-cell [value]="offering.endDate"
                              [cell-width]="endDateWidth">
              </formatted-cell>
              <formatted-cell [value]="offering.aatStatus"
                              [cell-width]="aatStatusWidth">
              </formatted-cell>
          </div>`
      })
    ]
  });
}
