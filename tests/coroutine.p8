pico-8 cartridge // http://www.pico-8.com
version 16
__lua__
x=4
y=4
cor = nil
 
function anim()
  for i=4,124,4 do
    x=i
    y=i
  end
end
 
function _update() 
  if btnp(5) then
    cor = cocreate(anim)
  end
  if cor and costatus(cor) != 'dead' then
    coresume(cor)
  else
    cor = nil
  end
end
 
function _draw()
  cls()
  circfill(x, y, 4, 7)
end
