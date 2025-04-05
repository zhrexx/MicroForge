class Element {
  constructor(name, attribs = {}, ...children) {
    this.name = name;
    this.attribs = {...attribs};
    this.children = [...children];
    this._events = {};
  }

  attr(key, value) {
    this.attribs[key] = value;
    return this;
  }

  attrs(attributes) {
    for (const [key, value] of Object.entries(attributes)) {
      this.attribs[key] = value;
    }
    return this;
  }

  child(child) {
    this.children.push(child);
    return this;
  }

  children(childrenArray) {
    this.children.push(...childrenArray);
    return this;
  }

  clearChildren() {
    this.children = [];
    return this;
  }

  on(eventName, handler) {
    const existingAttr = this.attribs[`on${eventName}`];
    if (existingAttr) {
      const oldHandler = new Function('event', existingAttr);
      this.attribs[`on${eventName}`] = `(${oldHandler})(event); (${handler})(event);`;
    } else {
      this.attribs[`on${eventName}`] = `(${handler})(event)`;
    }
    this._events[eventName] = handler;
    return this;
  }

  off(eventName) {
    delete this.attribs[`on${eventName}`];
    delete this._events[eventName];
    return this;
  }

  addClass(className) {
    const currentClasses = this.attribs.class ? this.attribs.class.split(' ') : [];
    if (!currentClasses.includes(className)) {
      currentClasses.push(className);
      this.attribs.class = currentClasses.join(' ');
    }
    return this;
  }

  removeClass(className) {
    if (this.attribs.class) {
      const currentClasses = this.attribs.class.split(' ');
      const index = currentClasses.indexOf(className);
      if (index !== -1) {
        currentClasses.splice(index, 1);
        this.attribs.class = currentClasses.join(' ');
      }
    }
    return this;
  }

  html(content) {
    this.clearChildren();
    if (typeof content === 'string') {
      this.child(content);
    }
    return this;
  }

  text(content) {
    this.clearChildren();
    this.child(String(content));
    return this;
  }

  css(property, value) {
    const currentStyle = this.attribs.style || '';
    const propertyName = property.replace(/([A-Z])/g, '-$1').toLowerCase();
    
    const styleObj = {};
    currentStyle.split(';').forEach(style => {
      const [prop, val] = style.split(':');
      if (prop && val) {
        styleObj[prop.trim()] = val.trim();
      }
    });
    
    styleObj[propertyName] = value;
    
    const newStyle = Object.entries(styleObj)
      .map(([prop, val]) => `${prop}: ${val}`)
      .join('; ');
      
    this.attribs.style = newStyle;
    return this;
  }

  render() {
    const el = document.createElement(this.name);
    
    for (const attr in this.attribs) {
      el.setAttribute(attr, this.attribs[attr]);
    }

    this.children.forEach(child => {
      if (child == null) return;
      
      if (typeof child === "string" || typeof child === "number") {
        el.appendChild(document.createTextNode(String(child)));
      } else if (child instanceof Element) {
        el.appendChild(child.render());
      } else if (child instanceof Node) {
        el.appendChild(child);
      }
    });

    return el;
  }
}

class StyleManager {
  constructor(prefix = 'xwui-') {
    this.styles = {};
    this.styleElement = null;
    this.prefix = prefix;
    this.animations = {};
  }
  
  init() {
    if (!this.styleElement) {
      this.styleElement = document.createElement('style');
      this.styleElement.id = `${this.prefix}styles`;
      document.head.appendChild(this.styleElement);
    }
  }
  
  create(name, properties) {
    const ruleName = this.prefix + name;
    this.styles[ruleName] = properties;
    this.updateStyleElement();
    return ruleName;
  }
  
  update(name, properties) {
    const ruleName = name.startsWith(this.prefix) ? name : this.prefix + name;
    if (this.styles[ruleName]) {
      this.styles[ruleName] = {...this.styles[ruleName], ...properties};
    } else {
      this.styles[ruleName] = properties;
    }
    this.updateStyleElement();
    return ruleName;
  }
  
  remove(name) {
    const ruleName = name.startsWith(this.prefix) ? name : this.prefix + name;
    if (this.styles[ruleName]) {
      delete this.styles[ruleName];
      this.updateStyleElement();
      return true;
    }
    return false;
  }
  
  createCSS(properties) {
    return Object.entries(properties)
      .map(([key, value]) => {
        const cssKey = key.replace(/([A-Z])/g, '-$1').toLowerCase();
        return `${cssKey}: ${value};`;
      })
      .join(' ');
  }
  
  updateStyleElement() {
    if (!this.styleElement) {
      this.init();
    }
    
    let cssText = '';
    for (const [ruleName, properties] of Object.entries(this.styles)) {
      cssText += `.${ruleName} { ${this.createCSS(properties)} }\n`;
    }
    
    for (const [animName, props] of Object.entries(this.animations)) {
      cssText += `@keyframes ${animName} {\n`;
      for (const [step, stepProps] of Object.entries(props)) {
        cssText += `  ${step} { ${this.createCSS(stepProps)} }\n`;
      }
      cssText += `}\n`;
    }
    
    this.styleElement.textContent = cssText;
  }
  
  apply(element, ruleName) {
    const fullRuleName = ruleName.startsWith(this.prefix) ? ruleName : this.prefix + ruleName;
    if (element instanceof Element) {
      const classes = element.attribs.class || '';
      if (!classes.includes(fullRuleName)) {
        element.attr('class', classes + (classes ? ' ' : '') + fullRuleName);
      }
    }
    return element;
  }
  
  unapply(element, ruleName) {
    const fullRuleName = ruleName.startsWith(this.prefix) ? ruleName : this.prefix + ruleName;
    if (element instanceof Element) {
      const classes = element.attribs.class || '';
      element.attr('class', classes.replace(new RegExp(`\\b${fullRuleName}\\b`, 'g'), '').trim());
    }
    return element;
  }
  
  animation(name, keyframes) {
    const animName = `${this.prefix}anim-${name}`;
    this.animations[animName] = keyframes;
    this.updateStyleElement();
    return animName;
  }
  
  applyAnimation(element, animName, duration = '1s', timing = 'ease', iterations = '1') {
    const fullAnimName = animName.startsWith(`${this.prefix}anim-`) ? animName : `${this.prefix}anim-${animName}`;
    const animationProperty = `${fullAnimName} ${duration} ${timing} ${iterations}`;
    
    if (element instanceof Element) {
      element.css('animation', animationProperty);
    }
    
    return element;
  }
  
  mediaQuery(query, rules) {
    if (!this.styleElement) {
      this.init();
    }
    
    let cssText = this.styleElement.textContent || '';
    cssText += `\n@media ${query} {\n`;
    
    for (const [selector, properties] of Object.entries(rules)) {
      cssText += `  ${selector} { ${this.createCSS(properties)} }\n`;
    }
    
    cssText += '}\n';
    
    this.styleElement.textContent = cssText;
  }
}

class State {
  constructor(initialState = {}) {
    this.state = { ...initialState };
    this.listeners = {};
    this.$ = new Proxy({}, {
      get: (target, prop) => {
        return this.state[prop];
      },
      set: (target, prop, value) => {
        this.set({ [prop]: value });
        return true;
      }
    });
  }
  
  get(key) {
    return key ? this.state[key] : { ...this.state };
  }
  
  set(updates) {
    const prevState = { ...this.state };
    this.state = { ...this.state, ...updates };
    
    for (const [key, value] of Object.entries(updates)) {
      if (prevState[key] !== value && this.listeners[`change:${key}`]) {
        this.listeners[`change:${key}`].forEach(listener => listener(value, prevState[key]));
      }
    }
    
    if (this.listeners.change) {
      this.listeners.change.forEach(listener => listener(this.state, prevState));
    }
    
    return this;
  }
  
  on(event, listener) {
    if (!this.listeners[event]) {
      this.listeners[event] = [];
    }
    this.listeners[event].push(listener);
    return this;
  }
  
  off(event, listener) {
    if (!this.listeners[event]) return this;
    if (listener) {
      this.listeners[event] = this.listeners[event].filter(l => l !== listener);
    } else {
      delete this.listeners[event];
    }
    return this;
  }
}

class Router {
  constructor(app) {
    this.app = app;
    this.routes = {};
    this.currentRoute = null;
    this.params = {};
    this.query = {};
    
    window.addEventListener('popstate', this.handlePopState.bind(this));
    
    if (document.readyState === 'complete') {
      this.handleInitialRoute();
    } else {
      window.addEventListener('load', this.handleInitialRoute.bind(this));
    }
  }
  
  add(path, handler) {
    this.routes[path] = handler;
    return this;
  }
  
  go(path, params = {}) {
    window.history.pushState({}, '', path);
    this.handleRoute(path, params);
    return this;
  }
  
  handlePopState() {
    this.handleRoute(window.location.pathname);
  }
  
  handleInitialRoute() {
    this.handleRoute(window.location.pathname);
  }
  
  handleRoute(path, extraParams = {}) {
    this.currentRoute = path;
    
    this.query = {};
    const queryString = window.location.search.substring(1);
    if (queryString) {
      queryString.split('&').forEach(pair => {
        const [key, value] = pair.split('=');
        this.query[decodeURIComponent(key)] = decodeURIComponent(value || '');
      });
    }
    
    this.params = { ...extraParams };
    
    let handler = this.routes[path];
    
    if (!handler) {
      const dynamicRoutes = Object.keys(this.routes).filter(route => route.includes(':'));
      
      for (const route of dynamicRoutes) {
        const routeParts = route.split('/');
        const pathParts = path.split('/');
        
        if (routeParts.length === pathParts.length) {
          let match = true;
          const params = {};
          
          for (let i = 0; i < routeParts.length; i++) {
            if (routeParts[i].startsWith(':')) {
              const paramName = routeParts[i].substring(1);
              params[paramName] = pathParts[i];
            } else if (routeParts[i] !== pathParts[i]) {
              match = false;
              break;
            }
          }
          
          if (match) {
            handler = this.routes[route];
            this.params = { ...this.params, ...params };
            break;
          }
        }
      }
    }
    
    if (handler) {
      this.app.clear();
      handler(this.params, this.query);
    } else if (this.routes['*']) {
      this.app.clear();
      this.routes['*'](this.params, this.query);
    }
  }
  
  current() {
    return this.currentRoute;
  }
  
  getParams() {
    return { ...this.params };
  }
  
  getQuery() {
    return { ...this.query };
  }
}

class Storage {
  constructor(prefix = 'xwui_') {
    this.prefix = prefix;
  }
  
  set(key, value) {
    try {
      const serializedValue = JSON.stringify(value);
      localStorage.setItem(`${this.prefix}${key}`, serializedValue);
      return true;
    } catch (error) {
      return false;
    }
  }
  
  get(key, defaultValue = null) {
    try {
      const serializedValue = localStorage.getItem(`${this.prefix}${key}`);
      return serializedValue === null ? defaultValue : JSON.parse(serializedValue);
    } catch (error) {
      return defaultValue;
    }
  }
  
  remove(key) {
    try {
      localStorage.removeItem(`${this.prefix}${key}`);
      return true;
    } catch (error) {
      return false;
    }
  }
  
  clear() {
    try {
      Object.keys(localStorage).forEach(key => {
        if (key.startsWith(this.prefix)) {
          localStorage.removeItem(key);
        }
      });
      return true;
    } catch (error) {
      return false;
    }
  }
  
  keys() {
    try {
      return Object.keys(localStorage)
        .filter(key => key.startsWith(this.prefix))
        .map(key => key.substring(this.prefix.length));
    } catch (error) {
      return [];
    }
  }
}

class Http {
  constructor(baseURL = '') {
    this.baseURL = baseURL;
    this.headers = {
      'Content-Type': 'application/json'
    };
  }
  
  setBaseURL(url) {
    this.baseURL = url;
    return this;
  }
  
  setHeader(key, value) {
    this.headers[key] = value;
    return this;
  }
  
  async request(method, endpoint, data = null, options = {}) {
    const url = `${this.baseURL}${endpoint}`;
    
    const requestOptions = {
      method,
      headers: { ...this.headers, ...options.headers },
      ...options
    };
    
    if (data) {
      requestOptions.body = JSON.stringify(data);
    }
    
    try {
      const response = await fetch(url, requestOptions);
      const contentType = response.headers.get('content-type');
      
      let result;
      if (contentType && contentType.includes('application/json')) {
        result = await response.json();
      } else {
        result = await response.text();
      }
      
      if (!response.ok) {
        throw {
          status: response.status,
          statusText: response.statusText,
          data: result
        };
      }
      
      return result;
    } catch (error) {
      throw error;
    }
  }
  
  get(endpoint, options = {}) {
    return this.request('GET', endpoint, null, options);
  }
  
  post(endpoint, data, options = {}) {
    return this.request('POST', endpoint, data, options);
  }
  
  put(endpoint, data, options = {}) {
    return this.request('PUT', endpoint, data, options);
  }
  
  patch(endpoint, data, options = {}) {
    return this.request('PATCH', endpoint, data, options);
  }
  
  delete(endpoint, options = {}) {
    return this.request('DELETE', endpoint, null, options);
  }
}

class XWUI {
  constructor(options = {}) {
    this.elements = [];
    this.root = null;
    this.title = options.title || 'XWUI App';
    this.onload = null;
    
    this.style = new StyleManager();
    this.state = new State();
    this.storage = new Storage();
    this.http = new Http();
    this.router = new Router(this);
    
    this.autoRender = options.autoRender !== false;
    this.onloadExecuted = false;
    
    window.addEventListener('load', this.init.bind(this));
  }

  init() {
    if (this.onloadExecuted) return;
    
    this.onloadExecuted = true;
    document.title = this.title;
    
    this.style.init();
    
    this.root = document.getElementById('root') || document.body;
    
    if (this.onload && typeof this.onload === 'function') {
      this.onload();
    }
    
    if (this.autoRender) {
      this.render();
    }
  }

  create(name, attribs = {}, ...children) {
    const elem = new Element(name, attribs, ...children);
    this.elements.push(elem);
    return elem;
  }
  
  remove(element) {
    this.elements = this.elements.filter(el => el !== element);
    return this;
  }
  
  clear() {
    this.elements = [];
    if (this.root) {
      this.root.innerHTML = '';
    }
    return this;
  }
  
  find(selector) {
    if (selector.startsWith('#')) {
      return this.elements.find(elem => elem.attribs.id === selector.substring(1)) || null;
    } else if (selector.startsWith('.')) {
      const className = selector.substring(1);
      return this.elements.find(elem => {
        const classes = elem.attribs.class ? elem.attribs.class.split(' ') : [];
        return classes.includes(className);
      }) || null;
    } else {
      return this.elements.find(elem => elem.name === selector) || null;
    }
  }
  
  findAll(selector) {
    if (selector.startsWith('#')) {
      const element = this.elements.find(elem => elem.attribs.id === selector.substring(1));
      return element ? [element] : [];
    } else if (selector.startsWith('.')) {
      const className = selector.substring(1);
      return this.elements.filter(elem => {
        const classes = elem.attribs.class ? elem.attribs.class.split(' ') : [];
        return classes.includes(className);
      });
    } else {
      return this.elements.filter(elem => elem.name === selector);
    }
  }

  render() {
    if (!this.root) return this;
    this.root.innerHTML = '';
    this.elements.forEach(elem => this.root.appendChild(elem.render()));
    return this;
  }
  
  setOnload(callback) {
    this.onload = callback;
    return this;
  }
}

class ElementFactories {
  constructor(app) {
    this.app = app;
  }

  create(tag, attribs, ...children) {
    return this.app.create(tag, attribs, ...children);
  }

  div(attribs, ...children) { return this.create("div", attribs, ...children); }
  span(attribs, ...children) { return this.create("span", attribs, ...children); }
  p(attribs, ...children) { return this.create("p", attribs, ...children); }
  h1(attribs, ...children) { return this.create("h1", attribs, ...children); }
  h2(attribs, ...children) { return this.create("h2", attribs, ...children); }
  h3(attribs, ...children) { return this.create("h3", attribs, ...children); }
  h4(attribs, ...children) { return this.create("h4", attribs, ...children); }
  h5(attribs, ...children) { return this.create("h5", attribs, ...children); }
  h6(attribs, ...children) { return this.create("h6", attribs, ...children); }
  ul(attribs, ...children) { return this.create("ul", attribs, ...children); }
  ol(attribs, ...children) { return this.create("ol", attribs, ...children); }
  li(attribs, ...children) { return this.create("li", attribs, ...children); }
  a(attribs, ...children) { return this.create("a", attribs, ...children); }
  img(attribs) { return this.create("img", attribs); }
  button(attribs, ...children) { return this.create("button", attribs, ...children); }
  input(attribs) { return this.create("input", attribs); }
  textarea(attribs, ...children) { return this.create("textarea", attribs, ...children); }
  form(attribs, ...children) { return this.create("form", attribs, ...children); }
  table(attribs, ...children) { return this.create("table", attribs, ...children); }
  tr(attribs, ...children) { return this.create("tr", attribs, ...children); }
  td(attribs, ...children) { return this.create("td", attribs, ...children); }
  th(attribs, ...children) { return this.create("th", attribs, ...children); }
  thead(attribs, ...children) { return this.create("thead", attribs, ...children); }
  tbody(attribs, ...children) { return this.create("tbody", attribs, ...children); }
  footer(attribs, ...children) { return this.create("footer", attribs, ...children); }
  header(attribs, ...children) { return this.create("header", attribs, ...children); }
  section(attribs, ...children) { return this.create("section", attribs, ...children); }
  article(attribs, ...children) { return this.create("article", attribs, ...children); }
  nav(attribs, ...children) { return this.create("nav", attribs, ...children); }
  aside(attribs, ...children) { return this.create("aside", attribs, ...children); }
  main(attribs, ...children) { return this.create("main", attribs, ...children); }
}

class Component {
  constructor(app, options = {}) {
    this.app = app;
    this.el = new window.gXWUI.ElementFactories(app);
    this.options = { ...options };
    this.state = new window.gXWUI.State();
    this.element = null;
    this.children = [];
    this.id = options.id || `component-${Date.now()}-${Math.floor(Math.random() * 1000)}`;
  }

  setState(updates) {
    this.state.set(updates);
    if (this.options.autoRender !== false) {
      this.update();
    }
    return this;
  }

  getState() {
    return this.state.get();
  }

  addChild(childComponent) {
    if (childComponent instanceof Component) {
      this.children.push(childComponent);
    }
    return this;
  }

  removeChild(childComponent) {
    this.children = this.children.filter(child => child !== childComponent);
    return this;
  }

  render() {
    // This method should be overridden by subclasses
    this.element = this.el.div({ id: this.id });
    return this.element;
  }

  update() {
    if (!this.element) {
      return this;
    }

    const parent = this.element.render().parentNode;
    if (!parent) {
      return this;
    }

    const newElement = this.render();
    parent.replaceChild(newElement.render(), this.element.render());
    return this;
  }

  mount(container) {
    if (typeof container === 'string') {
      const domContainer = document.querySelector(container);
      if (domContainer) {
        domContainer.innerHTML = '';
        domContainer.appendChild(this.render().render());
      }
    } else if (container instanceof Element) {
      container.clearChildren();
      container.child(this.render());
    }
    return this;
  }

  unmount() {
    if (this.element) {
      const domElement = document.getElementById(this.id);
      if (domElement && domElement.parentNode) {
        domElement.parentNode.removeChild(domElement);
      }
    }
    return this;
  }

  destroy() {
    this.unmount();
    this.children.forEach(child => child.destroy());
    this.children = [];
    this.element = null;
    return this;
  }

  on(event, callback) {
    this.state.on(event, callback);
    return this;
  }

  off(event, callback) {
    this.state.off(event, callback);
    return this;
  }

  emit(event, ...args) {
    if (this.state.listeners[event]) {
      this.state.listeners[event].forEach(listener => listener(...args));
    }
    return this;
  }
}

function createXWUI(options = {}) {
  const app = new XWUI(options);
  const el = new ElementFactories(app);
  
  return { app, el };
}

window.gXWUI = {
  create: createXWUI,
  Element,
  StyleManager,
  State,
  Router,
  Storage,
  Http,
  Component,
};
