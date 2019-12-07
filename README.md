# crickets - wireless mesh sound device

## esp8266 or esp32 based WMN

## (optional) teensy35/36 or arduino nano.. by i2c comm.

### some cricket use only espX.. + motor or relay etc..

espX 단계에서는.. json으로 대화를 한다.
어차피 painlessmesh 도 그렇고. 문자열기반이므로, 이부분 어렵지 않음.
나중에 어떤 espX 한대가 중계기가 되는 경우 브라우져의 js 에 연결도 스무쓰해서 좋다.

그런데, teensy 가 있는 경우에는..
즉, sampler 인 경우에는.. 뭔가 컨트롤이 오고 갈 필요가 있는데..
이 컨트롤은 지금까지.. i2c를 써왔었는데.
i2c는 커멘드의 구조를 매번 잡아야하고 이런저런 것들이 불편하다.
좀더 유연한 통신을 원하는 것 같기도 하다. 양방향시리얼통신이라던가.. (uart)

또한, 지금 UI를 짜는 경우에.. 즉 브라우져 말고.. 디스플레이중심장치말고. 좀더 연주중심장치를 인터페이스 그니까, 오르가넬레 / pd 연결하는 경우에
osc 가 가장 유력한데
그런경우에 어떤 espX 는 osc를 보내주면 좋겠다.. 라는 바램이 있긴하다.
serialize가 된다면 어떤 메세지던 상관없다 매쉬입장에서는..
그렇다면, 애초에.. json을 하지 말고. osc를 주고 받게 할 수도 있지 않을까? espX 단부터..
브라우져도 osc.js 가 있기때문에 인터페이스할수있고..
이렇게 되면 teensy도 CNMAT OSC 가 있고.. espX 도 같은 라이브러리가 서포트 하는 걸로 보이는데..

다만, OSCBundle을 serialize 할 수 있는 수단. 그리고, 성능적인 오버헤드가 어느정도인지.. 그런질문이 남는다.

OSCBundle은 SLIP으로 encoding 하게 되어있는데..그렇게 하면 몬가 좋은게 있겠나? ..
암튼 근데 그게.. serial 통신 기반으로 하게 되어있어서.. espX <-> teensy 사이에는 쓸수있겠지만.. espX mesh 안에서는 string 메세지로 통신하니까. 쓰기가 애매하다. 별도로 print 하는 방법으로 하면 좋겠지만.. printout을 할수 있게 되어있는지 잘 모르겠음. 불확실.

teensy 또는 espX <-> pc 사이에는 쓸수가 있고, 썼을때, 유용하다. osc 니까.

문제는 mesh 이후인데.. mesh가 웹서비스를 할경우에.. osc.js로 할수가 있겠지만.. 음..

한가지 어서 정하는게 좋을거 같은데....

mesh들 끼리 통신은 string 이다.
osc를 string으로 표현하면 딱 좋아.
osc를 slip으로 encoding 한다고 하는 것은 packet을 만든다는 건데..
사실 그럴필요도 없지 않나 싶다.
그냥 다 전개된 string으로 만드는 방법만 있다면, 모든 것이 osc 가 될수가 있는데.
그런 방법이 있으면 좋겠지만.....

그냥 지금 가능한 방법으로만 한다면..
json이 serialize가 되니까.. osc를 json으로 바꿔서 serialize해서 보내고 받고 하는 것이 한가지 방법.

osc 는 되게 쌈빡한 메세지인데.. 이걸 json으로 만들면.. 되게 지저분해진다. overhead가 많아. 간단하게 적으면 되는 건데.. 그냥 프린트하는 방법이 없을까? cnmat osc 에서.... 한번 조사해보자.

----

StringStream 이란걸 구해다가 중간에 변환기 처럼 사용해서, 테스트를 해봤는데... 변환은 되지만, mesh에 전송했을때 안가는 문제가 있었는데, 원인은 다음과 같다.

https://gitlab.com/painlessMesh/painlessMesh/issues/140

painlessmesh 에서.. 그냥 binary data를 보내는 걸 허용을 안하기때문에.. osc 메세지를 바로 보낼수가 없다.
base64 인코딩을 해야한다는데.. 그렇게 되면, integrity가 깨진다고나 할까.. (숫자들만 base64 를 할건지.. 전체다 할건지.. 암튼 지저분해지잖아.)
어차피 내부적으로 json을 이미 쓰고 있다는 걸까? .. 그런가보다..
그럼 그냥 json으로 번역하도록 하는게 맞는 걸까?..
그럼 이걸.. generally 할 수 있어야 할텐데..
osc --> json  json --> osc 자동..
그런걸 만들 수 있을까?
OSCBundle 이란게 있다면. 이거를 확장해서 ArduinoJson 으로 print 하게 만들수도 있을까..
아.. 쩝. 지저분하거나, 혹은 어렵네..
가능하긴 한데.. 어려움.. 흠. 그래서 좀 귀찮음..



----

그렇다면..
아예.. 모든것을 json으로 바꿀순 없을까?
예를들어서 pd만 특별하게 예외로 한다던가 하고...

https://github.com/residuum/PuRestJson/wiki/%5Bjson-decode%5D-And-%5Bjson-encode%5D:-JSON-Data-in-Pd

이런게 있는데.. 흥미롭긴하지만.. symbol 로 json string을 받아야하는데 comport로 받으니까 (ArduinoJson) 그렇게 안됨.. serial 통신이니까. slip encoding이라도 안하면.. event도 아니고.. packet이면 차라리 편한데...

----

결국 중간에 json <--> osc 변환을 하는게 가장 현명한 방법인데.. 이걸 generally 구현하려면, 좀 짜증날거 같고..
그냥 약식으로 해야 할거 같네.. 어차피.. 이게 .. json이든 머든.. 형식이 free하다고 해도 마냥 free하게 쓸수있는 것도 아님.

----

또는....... ESP-MESH 를 사용해서 전체적으로 다시 만드는 수가 있음. 즉, painlessmesh 를 버리는 거지.
ESP-MESH 에서는 binary 를 보낼수가 있음.
즉, OSCBundle 을 StringStream으로 담아서.. 보내고 받고 할 수 있음.

그러면.. 되나?
그러면.. 모든 곳에서 OSC 메세지를 사용할수가 있게 되나?

----

근데. 솔직히.. 그거는 좀.. 너무 큰 일이다. 지금도 시간 많이 썼는데.. 더 이상은 위험하게 하고 싶지 않고, 재미없어질듯.

하지만, 다음에 꼭 해보자...

장기적으로 보나.. 필요한 방향..
