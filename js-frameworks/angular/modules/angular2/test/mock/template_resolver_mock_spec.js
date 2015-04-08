import {
  beforeEach,
  ddescribe,
  describe,
  el,
  expect,
  iit,
  it,
} from 'angular2/test_lib';

import {MockTemplateResolver} from 'angular2/src/mock/template_resolver_mock';

import {Component} from 'angular2/src/core/annotations/annotations';
import {Template} from 'angular2/src/core/annotations/template';

import {isBlank} from 'angular2/src/facade/lang';

export function main() {
  describe('MockTemplateResolver', () => {
    var resolver;

    beforeEach(() => {
      resolver = new MockTemplateResolver();
    });

    describe('Template overriding', () => {
      it('should fallback to the default TemplateResolver when templates are not overridden', () => {
        var template = resolver.resolve(SomeComponent);
        expect(template.inline).toEqual('template');
        expect(template.directives).toEqual([SomeDirective]);
      });

      it('should allow overriding a template', () => {
        resolver.setTemplate(SomeComponent, new Template({inline: 'overridden template'}));
        var template = resolver.resolve(SomeComponent);
        expect(template.inline).toEqual('overridden template');
        expect(isBlank(template.directives)).toBe(true);

      });

      it('should not allow overriding a template after it has been resolved', () => {
        resolver.resolve(SomeComponent);
        expect(() => {
          resolver.setTemplate(SomeComponent, new Template({inline: 'overridden template'}));
        }).toThrowError('The component SomeComponent has already been compiled, its configuration can not be changed');
      });
    });

    describe('inline definition overriding', () => {
      it('should allow overriding the default Template', () => {
        resolver.setInlineTemplate(SomeComponent, 'overridden template');
        var template = resolver.resolve(SomeComponent);
        expect(template.inline).toEqual('overridden template');
        expect(template.directives).toEqual([SomeDirective]);
      });

      it('should allow overriding an overriden template', () => {
        resolver.setTemplate(SomeComponent, new Template({inline: 'overridden template'}));
        resolver.setInlineTemplate(SomeComponent, 'overridden template x 2');
        var template = resolver.resolve(SomeComponent);
        expect(template.inline).toEqual('overridden template x 2');
      });

      it('should not allow overriding a template after it has been resolved', () => {
        resolver.resolve(SomeComponent);
        expect(() => {
          resolver.setInlineTemplate(SomeComponent, 'overridden template');
        }).toThrowError('The component SomeComponent has already been compiled, its configuration can not be changed');
      });
    });


    describe('Directive overriding', () => {
      it('should allow overriding a directive from the default template', () => {
        resolver.overrideTemplateDirective(SomeComponent, SomeDirective, SomeOtherDirective);
        var template = resolver.resolve(SomeComponent);
        expect(template.directives.length).toEqual(1);
        expect(template.directives[0]).toBe(SomeOtherDirective);
      });

      it('should allow overriding a directive from an overriden template', () => {
        resolver.setTemplate(SomeComponent, new Template({directives: [SomeOtherDirective]}));
        resolver.overrideTemplateDirective(SomeComponent, SomeOtherDirective, SomeComponent);
        var template = resolver.resolve(SomeComponent);
        expect(template.directives.length).toEqual(1);
        expect(template.directives[0]).toBe(SomeComponent);
      });

      it('should throw when the overridden directive is not present', () => {
        resolver.overrideTemplateDirective(SomeComponent, SomeOtherDirective, SomeDirective);
        expect(() => { resolver.resolve(SomeComponent); })
            .toThrowError('Overriden directive SomeOtherDirective not found in the template of SomeComponent');
      });

      it('should not allow overriding a directive after its template has been resolved', () => {
        resolver.resolve(SomeComponent);
        expect(() => {
          resolver.overrideTemplateDirective(SomeComponent, SomeDirective, SomeOtherDirective);
        }).toThrowError('The component SomeComponent has already been compiled, its configuration can not be changed');
      });
    });
  });
}

@Component({selector: 'cmp'})
@Template({
  inline: 'template',
  directives: [SomeDirective],
})
class SomeComponent {
}

class SomeDirective {
}

class SomeOtherDirective {
}
